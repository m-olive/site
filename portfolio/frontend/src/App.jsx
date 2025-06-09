import React, { useState, useEffect, useRef } from "react";
import "./App.css";
import { io } from "socket.io-client";
import "xterm/css/xterm.css";
import { Terminal as XTerm } from "xterm";

import {
  Github,
  Linkedin,
  Mail,
  ExternalLink,
  Terminal,
  User,
  Briefcase,
  Phone,
} from "lucide-react";

const PortfolioSite = () => {
  const [activeSection, setActiveSection] = useState("home");

  const navItems = [
    { id: "home", label: "Home", icon: User },
    { id: "about", label: "About", icon: User },
    { id: "projects", label: "Projects", icon: Briefcase },
    { id: "terminal", label: "Terminal", icon: Terminal },
    { id: "contact", label: "Contact", icon: Phone },
  ];

  const HomeSection = () => (
    <section className="section home">
      <div className="home-content">
        <div className="avatar">
          <img src="selfie.jpg" alt="Photo of Matt" />
        </div>
        <div style={{ marginBottom: "1rem" }}></div>
        <h1 className="home-title">Matt Oliveira</h1>
        <h2 className="home-subtitle">Full Stack Developer</h2>
        <div style={{ marginBottom: "2rem" }}></div>
        <p className="home-description">
          This is a simple portfolio to highlight my projects. Navigate to
          Terminal to try my custom C shell.
        </p>
        <div style={{ marginBottom: "1rem" }}></div>
        <div className="social-links">
          <a href="https://github.com/m-olive" className="social-link">
            <Github size={32} />
          </a>
          <a
            href="https://www.linkedin.com/in/m-olive/"
            className="social-link"
          >
            <Linkedin size={32} />
          </a>
          <a href="mailto:matt.d.oliveira@gmail.com" className="social-link">
            <Mail size={32} />
          </a>
        </div>
      </div>
    </section>
  );

  const AboutSection = () => (
    <section className="section about">
      <div className="about-content">
        <h2 className="section-title">About Me</h2>
        <div style={{ marginBottom: "1rem" }}></div>
        <div className="about-grid">
          <div className="about-image">
            <User size={80} />
            <div style={{ marginBottom: "1rem" }}></div>
          </div>
          <div>
            <p className="about-text">
              I am a Master's student living in the greater Boston area looking
              for work and internships. I have a Bachelor of Science in Software
              Engineering from Western Governor's University. I am currently
              enrolled in a Master of Science in Computer Science with a
              specialization in Computing Systems program from the Georgia
              Institute of Technology.
            </p>
            <div style={{ marginBottom: "4rem" }}></div>
            <div className="skills-grid">
              <div className="skill-item">
                <h4 className="skill-title">Languages</h4>
                <div style={{ marginBottom: ".1rem" }}></div>
                <p className="skill-text">C, Java, Python</p>
              </div>
              <div style={{ marginBottom: "2rem" }}></div>
              <div className="skill-item">
                <h4 className="skill-title">Frameworks</h4>
                <div style={{ marginBottom: ".1rem" }}></div>
                <p className="skill-text">Spring Boot, Django</p>
              </div>
              <div style={{ marginBottom: "1.5rem" }}></div>
              <div className="skill-item">
                <h4 className="skill-title">Web Technologies</h4>
                <p className="skill-text">HTML5, CSS, Tailwind</p>
              </div>
              <div style={{ marginBottom: "1.5rem" }}></div>
              <div className="skill-item">
                <h4 className="skill-title">Tooling, Platforms, and OS</h4>
                <p className="skill-text">
                  MySQL/MariaDB, Git, Docker, Kubernetes, Linux
                </p>
              </div>
            </div>
          </div>
        </div>
      </div>
    </section>
  );

  const ProjectsSection = () => {
    const projects = [
      {
        title: "Interactive C Shell",
        description:
          "Custom Unix shell implementation with job control, piping, redirection, and command history.",
        tech: ["C", "Docker", "WebSocket"],
      },
      {
        title: "Placeholder",
        description:
          "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.",
        tech: ["React", "Node.js", "C"],
      },
      {
        title: "Placeholder",
        description:
          "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.",
        tech: ["Java", "Spring Boot"],
      },
    ];

    return (
      <section className="section projects">
        <div className="container">
          <h2 className="section-title">Projects</h2>
          <div className="projects-grid">
            {projects.map((project, index) => (
              <div key={index} className="project-card">
                <div className="project-image">
                  <Briefcase size={48} />
                </div>
                <div className="project-content">
                  <h3 className="project-title">{project.title}</h3>
                  <p className="project-description">{project.description}</p>
                  <div className="project-tech">
                    {project.tech.map((tech, techIndex) => (
                      <span key={techIndex} className="tech-tag">
                        {tech}
                      </span>
                    ))}
                  </div>
                  <div className="project-links">
                    <button className="project-link">
                      <Github size={16} />
                      <span>Code</span>
                    </button>
                    <button className="project-link">
                      <ExternalLink size={16} />
                      <span>Demo</span>
                    </button>
                  </div>
                </div>
              </div>
            ))}
          </div>
        </div>
      </section>
    );
  };
  const TerminalSection = () => {
    const terminalRef = useRef(null);
    const xtermRef = useRef(null);
    const [socket, setSocket] = useState(null);
    const [isConnected, setIsConnected] = useState(false);
    const [isShellRunning, setIsShellRunning] = useState(false);
    const [connectionError, setConnectionError] = useState("");

    useEffect(() => {
      const newSocket = io("http://localhost:3001", {
        transports: ["websocket", "polling"],
      });

      newSocket.on("connect", () => {
        console.log("Connected to WebSocket server");
        setIsConnected(true);
        setConnectionError("");
        setSocket(newSocket);
      });

      newSocket.on("disconnect", () => {
        console.log("Disconnected from WebSocket server");
        setIsConnected(false);
        setIsShellRunning(false);
      });

      newSocket.on("connect_error", (error) => {
        console.error("Connection error:", error);
        setConnectionError("Failed to connect to server.");
        setIsConnected(false);
      });

      newSocket.on("shell_output", (data) => {
        if (xtermRef.current) {
          xtermRef.current.write(data);
        }
      });

      return () => {
        if (newSocket) {
          newSocket.emit("end_shell");
          newSocket.disconnect();
        }
      };
    }, []);

    useEffect(() => {
      xtermRef.current = new XTerm({
        cursorBlink: true,
        fontFamily:
          "SF Mono, Monaco, Inconsolata, Roboto Mono, Consolas, Courier New, monospace",
        fontSize: 14,
      });
      xtermRef.current.open(terminalRef.current);

      xtermRef.current.onData((data) => {
        if (socket && isShellRunning) {
          socket.emit("shell_input", data);
        }
      });

      return () => {
        if (xtermRef.current) {
          xtermRef.current.dispose();
        }
      };
    }, [socket, isShellRunning]);

    const startShell = () => {
      if (socket && isConnected) {
        xtermRef.current.clear();
        xtermRef.current.writeln("Starting shell session...");
        setIsShellRunning(true);
        socket.emit("start_shell");
      }
    };

    const stopShell = () => {
      if (socket) {
        socket.emit("end_shell");
        setIsShellRunning(false);
        xtermRef.current.writeln("\r\n[Shell session ended by user]");
      }
    };

    return (
      <section className="section terminal">
        <div className="terminal-content">
          <h2 className="terminal-title">Interactive C Shell</h2>
          <p className="terminal-description">
            Experience my custom C shell running in a secure Docker container
          </p>

          <div className="terminal-status">
            <div
              className={`status-indicator ${isConnected ? "connected" : "disconnected"}`}
            >
              {isConnected ? "🟢 Connected" : "🔴 Disconnected"}
            </div>
            {connectionError && (
              <div className="connection-error">{connectionError}</div>
            )}
            {isConnected && !isShellRunning && (
              <button className="start-shell-btn" onClick={startShell}>
                Start Shell Session
              </button>
            )}
            {isShellRunning && (
              <button className="stop-shell-btn" onClick={stopShell}>
                Stop Session
              </button>
            )}
          </div>

          <div ref={terminalRef} className="terminal-window"></div>

          <div className="terminal-info-panel">
            <h3>Shell Features</h3>
            <div className="feature-list">
              <span className="feature-item">📁 Directory Navigation (cd)</span>
              <span className="feature-item">
                🔧 Job Control (jobs, fg, bg)
              </span>
              <span className="feature-item">
                📊 Command History (history, !!, !n)
              </span>
              <span className="feature-item">
                🔀 Pipes and Redirection (|, &gt;, &lt;)
              </span>
              <span className="feature-item">⚡ Background Jobs (&)</span>
              <span className="feature-item">🛡️ Secure Docker Container</span>
            </div>
          </div>
        </div>
      </section>
    );
  };

  const ContactSection = () => (
    <section className="section contact">
      <div className="container">
        <h2 className="section-title">Get In Touch</h2>
        <div className="contact-grid">
          <div className="contact-info">
            <h3>Let's Connect</h3>
            <p className="contact-description">
              I'm always interested in new opportunities and exciting projects.
              Feel free to reach out if you'd like to collaborate.
            </p>
            <div className="contact-item">
              <div className="contact-icon">
                <Mail size={20} />
              </div>
              <span className="contact-text">matt.d.oliveira@gmail.com</span>
            </div>
            <div className="contact-item">
              <div className="contact-icon">
                <Linkedin size={20} />
              </div>
              <span className="contact-text">linkedin.com/in/m-olive</span>
            </div>
            <div className="contact-item">
              <div className="contact-icon">
                <Github size={20} />
              </div>
              <span className="contact-text">github.com/m-olive</span>
            </div>
          </div>
          <div className="contact-form">
            <div className="form-group">
              <label className="form-label">Name</label>
              <input
                type="text"
                className="form-input"
                placeholder="Your name"
              />
            </div>
            <div className="form-group">
              <label className="form-label">Email</label>
              <input
                type="email"
                className="form-input"
                placeholder="your.email@example.com"
              />
            </div>
            <div className="form-group">
              <label className="form-label">Message</label>
              <textarea
                className="form-textarea"
                placeholder="Your message"
              ></textarea>
            </div>
            <button className="form-button">Send Message</button>
          </div>
        </div>
      </div>
    </section>
  );

  return (
    <div>
      <nav className="navbar">
        <div className="nav-container">
          <div className="nav-logo">Portfolio</div>
          <div className="nav-menu">
            {navItems.map(({ id, label, icon: Icon }) => (
              <button
                key={id}
                onClick={() => setActiveSection(id)}
                className={`nav-item ${activeSection === id ? "active" : ""}`}
              >
                <Icon size={18} />
                <span>{label}</span>
              </button>
            ))}
          </div>
        </div>
      </nav>
      <main className="main">
        {activeSection === "home" && <HomeSection />}
        {activeSection === "about" && <AboutSection />}
        {activeSection === "projects" && <ProjectsSection />}
        {activeSection === "terminal" && <TerminalSection />}
        {activeSection === "contact" && <ContactSection />}
      </main>
    </div>
  );
};

export default PortfolioSite;
