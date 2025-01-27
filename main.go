package main

import (
	"fmt"
	"iot/backend"
	"net/http"
)

func main() {
	mqttClient := backend.ConnectMQTTBroker()

	fs := http.FileServer(http.Dir("frontend"))
	http.Handle("/", fs)

	http.HandleFunc("/time", backend.TimeHandler)

	http.HandleFunc("/distance", backend.DistanceHandler(mqttClient))

	http.HandleFunc("/lights", backend.LightsHandler(mqttClient))

	fmt.Println("server is running on http://localhost:8080")
	if err := http.ListenAndServe("0.0.0.0:8080", nil); err != nil {
		fmt.Println("error starting server:", err)
	}

	select {}
}
