# In site/Dockerfile
FROM node:18-alpine

WORKDIR /app

# Copy backend
COPY portfolio/backend ./backend
# Copy frontend build (you'll need to build it first)
COPY portfolio/frontend/build ./frontend/build

# Install backend dependencies
WORKDIR /app/backend
RUN npm install

# Expose port
EXPOSE 3000

# Start the backend (which should serve the frontend)
CMD ["npm", "start"]
