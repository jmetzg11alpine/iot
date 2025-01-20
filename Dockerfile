# Use an official Go image as the build environment
FROM golang:1.23 as builder

# Set the working directory inside the container
WORKDIR /app

# Copy the go.mod and go.sum files first
COPY go.mod go.sum ./

# Download dependencies
RUN go mod download

# Copy the application source code
COPY main.go .
COPY backend/ ./backend/

# Build the application with static linking
RUN CGO_ENABLED=0 GOOS=linux go build -o server main.go

# Use a base image for both the Go app and Mosquitto broker
FROM ubuntu:20.04

# Install Mosquitto
RUN apt-get update && \
    apt-get install -y mosquitto && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Set up Mosquitto configuration (allow anonymous connections)
COPY config/mosquitto.conf /etc/mosquitto/mosquitto.conf
COPY config/certs/ca.crt /mosquitto/config/ca.crt
COPY config/certs/server.crt /mosquitto/config/server.crt
COPY config/certs/server.key /mosquitto/config/server.key
COPY config/mosquitto.acl /mosquitto/config/mosquitto.acl

# Set the working directory in the final image
WORKDIR /app

# Copy the built binary from the builder stage
COPY --from=builder /app/server /app/

# Copy the CA certificate for the Go application
COPY config/certs/ca.crt /config/ca.crt

# Copy the static frontend files
COPY frontend /app/frontend

# Expose the application port
EXPOSE 8080 8883

# Command to run the application
CMD ["sh", "-c", "mosquitto -c /etc/mosquitto/mosquitto.conf & /app/server"]
