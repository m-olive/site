const express = require("express");
const { createServer } = require("http");
const { Server } = require("socket.io");
const pty = require("node-pty");
const cors = require("cors");
const { spawn } = require("child_process");

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

// Start the chat server as a background process
let chatServerProcess = null;
const startChatServer = () => {
  const chatServerPath = "/app/chat/chat-server";
  try {
    chatServerProcess = spawn(chatServerPath, [], {
      stdio: "ignore",
    });
    console.log(`Chat server started (pid: ${chatServerProcess.pid})`);
    chatServerProcess.on("exit", (code, signal) => {
      console.log(`Chat server exited (code: ${code}, signal: ${signal})`);
      chatServerProcess = null;
    });
  } catch (error) {
    console.error(`Failed to start chat server: ${error.message}`);
  }
};
startChatServer();

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

  socket.on("start_shell", (opts) => {
    cleanupShellProcess(socket.id);

    const cols = opts?.cols || 80;
    const rows = opts?.rows || 24;

    console.log(`Starting shell process for socket ${socket.id} (${cols}x${rows})`);

    try {
      const shellProcess = pty.spawn("/app/shell/shell", [], {
        name: "xterm-color",
        cols,
        rows,
        cwd: "/home/user/filesystem",
        env: { ...process.env, TERM: "xterm-color" },
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

  socket.on("resize", ({ cols, rows }) => {
    const shellProcess = shellProcesses.get(socket.id);
    if (shellProcess && !shellProcess.killed && cols > 0 && rows > 0) {
      try {
        shellProcess.resize(cols, rows);
      } catch (error) {
        console.error(`Error resizing shell for socket ${socket.id}:`, error);
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

const gracefulShutdown = (signal) => {
  console.log(`Received ${signal}, cleaning up...`);

  for (const [socketId] of shellProcesses) {
    cleanupShellProcess(socketId);
  }

  if (chatServerProcess) {
    try {
      process.kill(chatServerProcess.pid, "SIGTERM");
      console.log("Chat server stopped");
    } catch (error) {
      console.error(`Error stopping chat server: ${error.message}`);
    }
  }

  httpServer.close(() => {
    console.log("Server closed");
    process.exit(0);
  });
};

process.on("SIGTERM", () => gracefulShutdown("SIGTERM"));
process.on("SIGINT", () => gracefulShutdown("SIGINT"));

const PORT = process.env.PORT || 3001;
const HOST = process.env.HOST || "0.0.0.0";

httpServer.listen(PORT, HOST, () => {
  console.log(`Server listening on ${HOST}:${PORT}`);
});
