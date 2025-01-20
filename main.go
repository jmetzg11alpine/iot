package main

import (
	"crypto/tls"
	"crypto/x509"
	"fmt"
	"net/http"
	"os"
	"strconv"
	"strings"
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
	// opts.AddBroker("tls://iot-white-pond-1937.fly.dev:8883")
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

	mqttClient.Subscribe("esp32/distance", 0, func(client mqtt.Client, msg mqtt.Message) {
		fmt.Printf("%s: %s\n", msg.Topic(), string(msg.Payload()))
	})

	// mqttClient.Subscribe("server/distance", 0, func(client mqtt.Client, msg mqtt.Message) {
	// 	fmt.Printf("%s: %s\n", msg.Topic(), string(msg.Payload()))
	// })

	fs := http.FileServer(http.Dir("frontend"))
	http.Handle("/", fs)

	http.HandleFunc("/time", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Access-Control-Allow-Origin", "*")
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

	http.HandleFunc("/distance", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		w.Header().Set("Access-Control-Allow-Methods", "GET, OPTIONS")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type")

		if r.Method == http.MethodOptions {
			w.WriteHeader(http.StatusNoContent)
			return
		}

		responseChan := make(chan string)

		token := mqttClient.Subscribe("server/distance", 0, func(client mqtt.Client, msg mqtt.Message) {
			responseChan <- string(msg.Payload()) // Send the message payload to the channel
		})
		token.Wait()
		if token.Error() != nil {
			http.Error(w, "Failed to subscribe to topic", http.StatusInternalServerError)
			fmt.Println("Error subscribing to topic:", token.Error())
		}

		topic := "esp32/distance"
		message := "get_distance"
		token = mqttClient.Publish(topic, 0, false, message)
		token.Wait()
		if token.Error() != nil {
			http.Error(w, "Failed to publish message", http.StatusInternalServerError)
			fmt.Println("Error publishing message:", token.Error())
			return
		}

		select {
		case response := <-responseChan:
			numbers := strings.Split(response, ", ")
			var sum float64
			var count int
			for _, numStr := range numbers {
				num, err := strconv.ParseFloat(numStr, 64)
				if err != nil {
					continue
				}
				sum += num
				count++
			}

			avg := 0.0
			if count > 0 {
				avg = sum / float64(count)
			}
			w.Header().Set("Content-Type", "application/json")
			w.WriteHeader(http.StatusOK)
			fmt.Fprintf(w, `{"distance": %.2f}`, avg)
		case <-time.After(5 * time.Second): // Timeout if no response is received
			http.Error(w, "ESP32 response timed out", http.StatusGatewayTimeout)
		}
		mqttClient.Unsubscribe("server/distance")
	})

	fmt.Println("server is running on http://localhost:8080")
	if err := http.ListenAndServe("0.0.0.0:8080", nil); err != nil {
		fmt.Println("error starting server:", err)
	}

	select {}
}
