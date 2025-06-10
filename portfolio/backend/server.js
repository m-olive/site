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
});

app.use(cors());
app.use(express.json());
app.use(express.static("public"));

let shellProcess = null;

io.on("connection", (socket) => {
  console.log("Client connected");

  socket.on("start_shell", () => {
    if (shellProcess) {
      console.log("Shell already running");
      return;
    }

    console.log("Starting shell process");

    shellProcess = pty.spawn("/app/shell/shell", [], {
      name: "xterm-color",
      cwd: "/root/filesystem",
      env: process.env,
    });

    shellProcess.on("data", (data) => {
      socket.emit("shell_output", data);
    });

    shellProcess.on("exit", (code) => {
      console.log(`Shell process exited with code ${code}`);
      socket.emit("shell_output", `\n[Shell exited with code ${code}]`);
      shellProcess = null;
    });
  });

  socket.on("shell_input", (input) => {
    if (shellProcess) {
      shellProcess.write(input);
    }
  });

  socket.on("end_shell", () => {
    if (shellProcess) {
      shellProcess.kill();
      shellProcess = null;
    }
  });

  socket.on("disconnect", () => {
    console.log("Client disconnected");
    if (shellProcess) {
      shellProcess.kill();
      shellProcess = null;
    }
  });
});

httpServer.listen("3001", () => {
  console.log(`Server listening on port 3001`);
});
