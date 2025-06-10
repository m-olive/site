FROM gcc:13 AS shell-builder

WORKDIR /build
COPY shell/shell.c .
RUN gcc -o shell shell.c

FROM node:18

WORKDIR /app

COPY package.json package-lock.json ./
RUN npm install

COPY server.js ./

RUN mkdir -p /root/filesystem
COPY filesystem/ /root/filesystem/

RUN mkdir -p /app/shell
COPY --from=shell-builder /build/shell /app/shell/shell
RUN chmod +x /app/shell/shell

EXPOSE 3001
CMD ["node", "server.js"]

