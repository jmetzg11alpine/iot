# Use an official Go image as the build environment
FROM golang:1.20 as builder

# Set the working directory inside the container
WORKDIR /app

# Copy the application source code
COPY main.go .

# Build the application with static linking
RUN CGO_ENABLED=0 GOOS=linux go build -o server main.go

# Use a minimal distroless base image
FROM gcr.io/distroless/static:nonroot

# Set the working directory in the final image
WORKDIR /app

# Copy the built binary from the builder stage
COPY --from=builder /app/server /app/

# Copy the static frontend files
COPY frontend /app/frontend

# Expose the application port
EXPOSE 8080

# Command to run the application
CMD ["/app/server"]
