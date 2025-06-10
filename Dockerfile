FROM node:18-alpine AS frontend-build
WORKDIR /app/frontend
COPY portfolio/frontend/package*.json ./
RUN npm install
COPY portfolio/frontend ./
RUN npm run build

FROM node:18-alpine AS production
WORKDIR /app

COPY portfolio/backend/package*.json ./
RUN npm install
COPY portfolio/backend ./

COPY --from=frontend-build /app/frontend/build ./public

EXPOSE 3000
CMD ["npm", "start"]
