extends Node2D

@onready var network_manager = $NetworkManager
@onready var connect_button = $UI/ConnectButton
@onready var status_label = $UI/StatusLabel

func _ready():
	print("[GDScript] _ready called")
	connect_button.pressed.connect(_on_connect_pressed)
	status_label.text = "Ready to connect"
	print("[GDScript] Button signal connected")

func _on_connect_pressed():
	print("[GDScript] Connect button pressed!")
	status_label.text = "Connecting..."
	print("[GDScript] About to call connect_to_server")
	network_manager.connect_to_server("127.0.0.1:9000")
	print("[GDScript] connect_to_server returned")
