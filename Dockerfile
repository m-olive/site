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

FROM gcc:13 AS chat-builder

RUN apt-get update && apt-get install -y libncurses-dev && rm -rf /var/lib/apt/lists/*
WORKDIR /build
COPY chat/server.c chat/server.h chat/client.c ./
RUN gcc -Wall -o chat-server server.c && gcc -Wall -o chat client.c -lncurses

FROM node:18 AS production

WORKDIR /app

COPY portfolio/backend/package*.json ./
RUN npm install

COPY portfolio/backend ./

COPY --from=frontend-build /app/frontend/dist ./public

RUN mkdir -p /app/shell
COPY --from=shell-builder /build/shell /app/shell/shell
RUN chmod +x /app/shell/shell

RUN apt-get update && apt-get install -y libncurses6 msmtp msmtp-mta && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /app/chat
COPY --from=chat-builder /build/chat-server /app/chat/chat-server
COPY --from=chat-builder /build/chat /usr/bin/chat
RUN chmod +x /app/chat/chat-server /usr/bin/chat

COPY msmtprc /etc/msmtprc
RUN chmod 644 /etc/msmtprc

RUN mkdir -p /home/user/filesystem
COPY portfolio/backend/filesystem/ /home/user/filesystem/
RUN mkdir -p /home/user/filesystem/folder1 /home/user/filesystem/folder2
RUN useradd -ms /bin/bash user && chown -R user:user /home/user/filesystem

USER user

WORKDIR /app

EXPOSE 3001

CMD ["npm", "start"]
