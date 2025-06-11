FROM node:18 AS frontend-build

WORKDIR /app/frontend

RUN apt-get update && apt-get install -y \
  python3 \
  make \
  g++ \
  && ln -s /usr/bin/python3 /usr/bin/python \
  && rm -rf /var/lib/apt/lists/*

COPY portfolio/frontend/package*.json ./
RUN npm install --legacy-peer-deps
COPY portfolio/frontend ./
RUN npm run build

FROM gcc:13 AS shell-builder

WORKDIR /build
COPY portfolio/backend/shell/shell.c .
RUN gcc -o shell shell.c

FROM node:18 AS production

WORKDIR /app

COPY portfolio/backend/package*.json ./
RUN npm install

COPY portfolio/backend ./

COPY --from=frontend-build /app/frontend/dist ./public

RUN mkdir -p /app/shell
COPY --from=shell-builder /build/shell /app/shell/shell
RUN chmod +x /app/shell/shell

RUN mkdir -p /root/filesystem
COPY portfolio/backend/filesystem/ /root/filesystem/

EXPOSE 3001

CMD ["npm", "start"]
