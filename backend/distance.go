package backend

import (
	"encoding/json"
	"fmt"
	"net/http"
	"strconv"
	"strings"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
)

// DistanceHandler handles the /distance endpoint
func DistanceHandler(mqttClient mqtt.Client) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		w.Header().Set("Access-Control-Allow-Methods", "GET, OPTIONS")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type")

		if r.Method == http.MethodOptions {
			w.WriteHeader(http.StatusNoContent)
			return
		}

		responseChan := make(chan string)

		// Subscribe to the topic
		token := mqttClient.Subscribe("server/distance", 0, func(client mqtt.Client, msg mqtt.Message) {
			responseChan <- string(msg.Payload()) // Send the message payload to the channel
		})
		token.Wait()
		if token.Error() != nil {
			http.Error(w, "Failed to subscribe to server/distance", http.StatusInternalServerError)
			fmt.Println("Error subscribing to server/distance:", token.Error())
			return
		}

		// Publish a message to request distance
		topic := "esp32/distance"
		message := "get_distance"
		token = mqttClient.Publish(topic, 0, false, message)
		token.Wait()
		if token.Error() != nil {
			http.Error(w, "Failed to publish message", http.StatusInternalServerError)
			fmt.Println("Error publishing message:", token.Error())
			return
		}

		// Wait for the response or timeout
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

			// Send the response back to the client
			w.Header().Set("Content-Type", "application/json")
			w.WriteHeader(http.StatusOK)
			json.NewEncoder(w).Encode(map[string]float64{"distance": avg})
		case <-time.After(5 * time.Second): // Timeout if no response is received
			http.Error(w, "ESP32 response timed out", http.StatusGatewayTimeout)
		}

		// Unsubscribe from the topic
		mqttClient.Unsubscribe("server/distance")
	}
}
