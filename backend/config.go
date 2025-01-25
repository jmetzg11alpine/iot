package backend

import (
	"crypto/tls"
	"crypto/x509"
	"fmt"
	"net/http"
	"os"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
)

func ConnectMQTTBroker() mqtt.Client {
	certpool := x509.NewCertPool()
	caCert, err := os.ReadFile(CaCertPath)
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
	opts.AddBroker(TlsConnection)
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

func TimeHandler(w http.ResponseWriter, r *http.Request) {
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
}
