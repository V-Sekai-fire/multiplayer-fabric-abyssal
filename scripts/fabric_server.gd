# fabric_server.gd
# Headless FabricMultiplayerPeer zone server.
# One WebTransportPeer on zone_port serves all channels via QUIC stream
# multiplexing — HOL-free because QUIC streams don't block each other.
# Paths: "/" (game clients), "/ch1" "/ch2" "/ch3" (inter-zone channels).
# Mirrors the picoquic demo path_item_list[] pattern.

extends Node

@export var zone_port: int = 7443

var _peer: FabricMultiplayerPeer
var _wt: WebTransportPeer


func _ready() -> void:
	var crypto := Crypto.new()
	var key := crypto.generate_ecdsa()
	if not key:
		push_error("FabricServer: generate_ecdsa failed"); return

	var now := int(Time.get_unix_time_from_system())
	var san := PackedStringArray(["DNS:localhost", "IP:127.0.0.1", "IP:::1"])
	var cert := crypto.generate_self_signed_certificate_san(
		key, "CN=fabric-zone", _fmt(now), _fmt(now + 13 * 86400), san)
	if not cert:
		push_error("FabricServer: cert gen failed"); return

	var ctx := HashingContext.new()
	ctx.start(HashingContext.HASH_SHA256)
	ctx.update(cert.get_der())
	var hash := Marshalls.raw_to_base64(ctx.finish())

	# Single WebTransportPeer server — registers "/", "/ch1", "/ch2", "/ch3"
	# in the picoquic path_table (4-entry, matching the demo pattern).
	_wt = WebTransportPeer.new()
	if _wt.create_server(zone_port, "/", cert, key) != OK:
		push_error("FabricServer: WebTransportPeer.create_server(%d) failed" % zone_port)
		return

	print(JSON.stringify({"event": "ready", "port": zone_port, "cert_hash": hash}))

	# FabricMultiplayerPeer factory: all 4 channel calls return the same _wt.
	# Channel paths are handled server-side by the path_table; client-side
	# FabricMultiplayerPeer connects each channel to the matching path.
	_peer = FabricMultiplayerPeer.new()
	_peer.game_id = "abyssal_vr"
	_peer.server_factory = func(_port: int) -> MultiplayerPeer:
		return _wt

	if _peer.create_server(zone_port) != OK:
		push_error("FabricServer: FabricMultiplayerPeer.create_server failed")
		return

	print(JSON.stringify({"event": "listening", "port": zone_port}))


func _process(_delta: float) -> void:
	if _peer:
		_peer.poll()


static func _fmt(unix_time: int) -> String:
	var dt := Time.get_datetime_dict_from_unix_time(unix_time)
	return "%04d%02d%02d%02d%02d%02d" % [
		dt["year"], dt["month"], dt["day"],
		dt["hour"], dt["minute"], dt["second"]]
