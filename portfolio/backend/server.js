const express = require("express");
const { createServer } = require("http");
const { Server } = require("socket.io");
const pty = require("node-pty");
const cors = require("cors");

const app = express();
const httpServer = createServer(app);
const io = new Server(httpServer, {
  cors: {
    origin: "*",
    methods: ["GET", "POST"],
  },
  pingTimeout: 60000,
  pingInterval: 25000,
});

app.use(cors());
app.use(express.json());
app.use(express.static("public"));

const shellProcesses = new Map();

const cleanupShellProcess = (socketId) => {
  const shellProcess = shellProcesses.get(socketId);
  if (shellProcess && !shellProcess.killed) {
    try {
      shellProcess.kill("SIGTERM");
      console.log(`Cleaned up shell process for socket ${socketId}`);
    } catch (error) {
      console.error(
        `Error killing shell process for socket ${socketId}:`,
        error,
      );
    }
  }
  shellProcesses.delete(socketId);
};

io.on("connection", (socket) => {
  console.log(`Client connected: ${socket.id}`);

  socket.on("start_shell", () => {
    // Clean up any existing shell process for this socket
    cleanupShellProcess(socket.id);

    console.log(`Starting shell process for socket ${socket.id}`);

    try {
      const shellProcess = pty.spawn("/app/shell/shell", [], {
        name: "xterm-color",
        cwd: "/root/filesystem",
        env: process.env,
      });

      shellProcesses.set(socket.id, shellProcess);

      shellProcess.on("data", (data) => {
        if (socket.connected) {
          socket.emit("shell_output", data);
        }
      });

      shellProcess.on("exit", (code, signal) => {
        console.log(
          `Shell process exited with code ${code}, signal ${signal} for socket ${socket.id}`,
        );
        if (socket.connected) {
          socket.emit("shell_output", `\n[Shell exited with code ${code}]`);
        }
        shellProcesses.delete(socket.id);
      });

      shellProcess.on("error", (error) => {
        console.error(`Shell process error for socket ${socket.id}:`, error);
        if (socket.connected) {
          socket.emit("shell_output", `\n[Shell error: ${error.message}]`);
        }
        cleanupShellProcess(socket.id);
      });

      if (socket.connected) {
        socket.emit("shell_output", "Shell started successfully\r\n");
      }
    } catch (error) {
      console.error(`Failed to start shell for socket ${socket.id}:`, error);
      if (socket.connected) {
        socket.emit(
          "shell_output",
          `\n[Failed to start shell: ${error.message}]`,
        );
      }
    }
  });

  socket.on("shell_input", (input) => {
    const shellProcess = shellProcesses.get(socket.id);
    if (shellProcess && !shellProcess.killed) {
      try {
        shellProcess.write(input);
      } catch (error) {
        console.error(`Error writing to shell for socket ${socket.id}:`, error);
        if (socket.connected) {
          socket.emit(
            "shell_output",
            `\n[Shell input error: ${error.message}]`,
          );
        }
      }
    }
  });

  socket.on("end_shell", () => {
    console.log(`Ending shell for socket ${socket.id}`);
    cleanupShellProcess(socket.id);
  });

  socket.on("disconnect", (reason) => {
    console.log(`Client disconnected: ${socket.id}, reason: ${reason}`);
    cleanupShellProcess(socket.id);
  });

  socket.on("error", (error) => {
    console.error(`Socket error for ${socket.id}:`, error);
  });
});

process.on("SIGTERM", () => {
  console.log("Received SIGTERM, cleaning up...");

  for (const [socketId, shellProcess] of shellProcesses) {
    cleanupShellProcess(socketId);
  }

  httpServer.close(() => {
    console.log("Server closed");
    process.exit(0);
  });
});

process.on("SIGINT", () => {
  console.log("Received SIGINT, cleaning up...");

  for (const [socketId, shellProcess] of shellProcesses) {
    cleanupShellProcess(socketId);
  }

  httpServer.close(() => {
    console.log("Server closed");
    process.exit(0);
  });
});

const PORT = process.env.PORT || 3001;
httpServer.listen(PORT, () => {
  console.log(`Server listening on port ${PORT}`);
});
