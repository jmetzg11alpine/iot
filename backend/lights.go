package backend

import (
	"encoding/json"
	"fmt"
	"io"
	"net/http"

	mqtt "github.com/eclipse/paho.mqtt.golang"
)

type LightsRequest struct {
	Lights string `json:"lights"`
}

func LightsHandler(mqttClient mqtt.Client) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		w.Header().Set("Access-Control-Allow-Methods", "POST, OPTIONS")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type")
		w.Header().Set("Content-Type", "application/json")
		if r.Method == http.MethodOptions {
			w.WriteHeader(http.StatusNoContent)
			return
		}

		body, err := io.ReadAll(r.Body)
		if err != nil {
			http.Error(w, "Failed to read body", http.StatusInternalServerError)
		}
		defer r.Body.Close()

		var req LightsRequest
		if err := json.Unmarshal(body, &req); err != nil {
			http.Error(w, "Invalid JSON", http.StatusBadRequest)
		}

		fmt.Println("Received lights:", req.Lights)

		// publish message to esp32
		token := mqttClient.Publish("esp32/lights", 0, false, req.Lights)
		token.Wait()
		if token.Error() != nil {
			http.Error(w, "Failed to publish to esp32/lights", http.StatusInternalServerError)
			fmt.Println("Error publishing to esp32/lights", token.Error())
			return
		}

		response := map[string]string{"message": "lights activated"}
		w.WriteHeader(http.StatusOK)
		json.NewEncoder(w).Encode(response)
	}
}
