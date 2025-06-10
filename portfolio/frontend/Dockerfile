FROM node:22

RUN apt-get update && apt-get install -y \
  python3 \
  make \
  g++ \
  && ln -s /usr/bin/python3 /usr/bin/python \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY package.json package-lock.json ./
RUN npm install --legacy-peer-deps

COPY . .

EXPOSE 5173

CMD ["npm", "run", "dev"]

