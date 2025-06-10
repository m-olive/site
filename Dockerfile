FROM node:18 AS frontend-build
WORKDIR /app/frontend

RUN apt-get update && apt-get install -y \
  python3 \
  make \
  g++ \
  && ln -s /usr/bin/python3 /usr/bin/python \
  && rm -rf /var/lib/apt/lists/*

COPY portfolio/frontend/package*.json ./
RUN npm install
COPY portfolio/frontend ./
RUN npm run build

FROM node:18 AS production
WORKDIR /app

RUN apt-get update && apt-get install -y \
  python3 \
  make \
  g++ \
  && ln -s /usr/bin/python3 /usr/bin/python \
  && rm -rf /var/lib/apt/lists/*

COPY portfolio/backend/package*.json ./
RUN npm install
COPY portfolio/backend ./

COPY --from=frontend-build /app/frontend/build ./public

EXPOSE 3000
CMD ["npm", "start"]

