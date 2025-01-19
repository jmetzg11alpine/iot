package main

import (
	"crypto/tls"
	"crypto/x509"
	"fmt"
	"net/http"
	"os"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
)

func connectMQTTBroker() mqtt.Client {
	certpool := x509.NewCertPool()
	caCertPath := "/config/ca.crt" // Update with your actual CA cert path
	caCert, err := os.ReadFile(caCertPath)
	if err != nil {
		panic(fmt.Sprintf("Failed to load CA certificate: %v", err))
	}
	if !certpool.AppendCertsFromPEM(caCert) {
		panic("Failed to append CA certificate")
	}

	// Configure TLS
	tlsConfig := &tls.Config{
		RootCAs: certpool,
	}

	// Create MQTT client options
	opts := mqtt.NewClientOptions()
	opts.AddBroker("tls://localhost:8883")
	opts.SetClientID("go-server")
	opts.SetTLSConfig(tlsConfig)

	// Connect to the broker
	var client mqtt.Client
	for {
		client = mqtt.NewClient(opts)
		if token := client.Connect(); token.Wait() && token.Error() == nil {
			fmt.Println("Connected to MQTT broker")
			break
		} else {
			fmt.Println("Error connecting to MQTT broker:", token.Error())
			time.Sleep(2 * time.Second) // Retry every 2 seconds
		}
	}
	return client
}

func main() {
	mqttClient := connectMQTTBroker()

	mqttClient.Subscribe("esp32/trigger", 0, func(client mqtt.Client, msg mqtt.Message) {
		fmt.Printf("Received message on topic %s: %s\n", msg.Topic(), string(msg.Payload()))
	})

	fs := http.FileServer(http.Dir("frontend"))
	http.Handle("/", fs)

	http.HandleFunc("/time", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Access-Control-Allow-Origin", "*") // Allow all origins
		w.Header().Set("Access-Control-Allow-Methods", "GET, OPTIONS")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type")

		if r.Method == http.MethodOptions {
			w.WriteHeader(http.StatusNoContent)
			return
		}
		currentTime := time.Now().Format("15:04:05")
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusOK)
		fmt.Fprintf(w, `{"time": "%s"}`, currentTime)
	})

	http.HandleFunc("/trigger", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Access-Control-Allow-Origin", "*") // Allow all origins
		w.Header().Set("Access-Control-Allow-Methods", "GET, OPTIONS")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type")

		if r.Method == http.MethodOptions {
			w.WriteHeader(http.StatusNoContent)
			return
		}
		topic := "esp32/trigger"
		message := "Hello from Go"
		token := mqttClient.Publish(topic, 0, false, message)
		token.Wait()

		if token.Error() != nil {
			http.Error(w, "Failed to publish message", http.StatusInternalServerError)
			fmt.Println("Error publishing message:", token.Error())
			return
		}

		// Respond to the HTTP request
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusOK)
		fmt.Fprint(w, `{"status": "trigger sent"}`)

	})

	fmt.Println("server is running on http://localhost:8080")
	if err := http.ListenAndServe("0.0.0.0:8080", nil); err != nil {
		fmt.Println("error starting server:", err)
	}

	select {}
}
