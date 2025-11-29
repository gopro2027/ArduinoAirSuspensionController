#include "WebOTA.h"
#include <Arduino.h>
#include <WiFiClient.h>

#ifdef ESP32
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <WiFi.h>

WebServer OTAServer(9999);
#endif

WebOTA webota;

char WWW_USER[16] = "";
char WWW_PASSWORD[16] = "";
const char *WWW_REALM = "WebOTA";
// the Content of the HTML response in case of Unautherized Access Default:empty
String authFailResponse = "Auth Fail";

////////////////////////////////////////////////////////////////////////////

int WebOTA::init(const unsigned int port, const char *path)
{
	this->port = port;
	this->path = path;

	// Only run this once
	if (this->init_has_run)
	{
		return 0;
	}

	add_http_routes(&OTAServer, path);

#ifdef ESP32
	// Fix some slowness
	OTAServer.enableDelay(false);
#endif
	OTAServer.begin(port);

	// Store that init has already run
	this->init_has_run = true;

	return 1;
}

// One param
int WebOTA::init(const unsigned int port)
{
	return WebOTA::init(port, "/webota");
}

// No params
int WebOTA::init()
{
	return WebOTA::init(8080, "/webota");
}

void WebOTA::useAuth(const char *user, const char *password)
{
	strncpy(WWW_USER, user, sizeof(WWW_USER) - 1);
	strncpy(WWW_PASSWORD, password, sizeof(WWW_PASSWORD) - 1);

	// Serial.printf("Set auth '%s' / '%s' %d\n", user, password, len);
}

int WebOTA::handle()
{
	// If we haven't run the init yet run it
	if (!this->init_has_run)
	{
		WebOTA::init();
	}

	OTAServer.handleClient();

	return 1;
}

long WebOTA::max_sketch_size()
{
	long ret = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;

	return ret;
}

// R Macro string literal https://en.cppreference.com/w/cpp/language/string_literal
const char INDEX_HTML[] PROGMEM = R"!^!(
<!doctype html>
<html lang="en">
<head>
	<meta charset="utf-8">
	<meta name="viewport" content="width=device-width, initial-scale=1">
	<title>OASMan OTA Update</title>

	<link rel="stylesheet" href="main.css">
	<script src="main.js"></script>
</head>

<body>
	<div class="card">
		<h1>OASMan OTA Update</h1>

		<form method="POST" action="#" enctype="multipart/form-data" id="upload_form">
			<label class="file_input">
				<input type="file" name="update" id="file">
				<span>Select firmware...</span>
			</label>

			<button class="btn" type="submit">Upload</button>
		</form>

		<p class="info">
			<b>Max sketch size:</b> %s<br>
			<b>Board:</b> %s<br>
			<b>MAC:</b> %s<br>
			<b>Uptime:</b> %s<br>
		</p>

		<div class="prog_wrap">
			<div id="prg" class="prog_bar"></div>
		</div>
	</div>
</body>
</html>
)!^!";

const char MAIN_CSS[] PROGMEM = R"!^!(
body {
	margin: 0;
	padding: 0;
	font-family: system-ui, sans-serif;
	background: #f2f5f7;
	display: flex;
	justify-content: center;
	align-items: flex-start;
	min-height: 100vh;
	padding-top: 3rem;
	color: #222;
}

.card {
	background: #fff;
	padding: 2.5rem;
	border-radius: 12px;
	box-shadow: 0 4px 20px rgba(0,0,0,0.08);
	max-width: 400px;
	width: 90%;
}

h1 {
	font-size: 1.6rem;
	font-weight: 600;
	margin-bottom: 1.4rem;
	text-align: center;
}

.file_input {
	display: block;
	margin-bottom: 1rem;
	position: relative;
	cursor: pointer;
	background: #eef2f4;
	padding: 0.9rem;
	border-radius: 8px;
	text-align: center;
	font-size: 0.95rem;
	color: #333;
	transition: background .2s ease;
}

.file_input:hover {
	background: #e1e6ea;
}

.file_input input[type=file] {
	opacity: 0;
	position: absolute;
	inset: 0;
	cursor: pointer;
}

.btn {
	width: 100%;
	padding: 0.9rem;
	border: none;
	border-radius: 8px;
	font-size: 1rem;
	cursor: pointer;
	background: #2d6ca2;
	color: #fff;
	font-weight: 500;
	transition: opacity .2s ease;
	margin-top: .3rem;
}

.btn:hover {
	opacity: 0.85;
}

.info {
	font-size: 0.9rem;
	margin-top: 1.4rem;
	line-height: 1.5;
	color: #444;
}

.prog_wrap {
	margin-top: 1rem;
	width: 100%;
	background: #e2e2e2;
	border-radius: 8px;
	overflow: hidden;
	display: none;
}

.prog_bar {
	width: 0%;
	background: #2d6ca2;
	color: #fff;
	text-align: center;
	padding: .5rem 0;
	font-size: 1rem;
	transition: width .15s linear;
}
)!^!";

const char MAIN_JS[] PROGMEM = R"!^!(
document.addEventListener("DOMContentLoaded", () => {
	const form = document.getElementById('upload_form');
	const fileInput = document.getElementById('file');
	const prg = document.getElementById('prg');
	const prgWrap = document.querySelector('.prog_wrap');

	form.addEventListener("submit", (event) => {
		event.preventDefault();

		const file = fileInput.files[0];
		if (!file) return;

		const formData = new FormData();
		formData.append("files", file, file.name);

		const xhr = new XMLHttpRequest();

		xhr.upload.addEventListener("progress", (evt) => {
			if (!evt.lengthComputable) return;

			const percent = Math.round((evt.loaded / evt.total) * 100);
			prg.style.width = percent + "%";
			prg.textContent = percent + "%";

			prgWrap.style.display = "block";
		});

		xhr.open("POST", location.href, true);
		xhr.send(formData);
	});
});
)!^!";

String WebOTA::get_board_type()
{

	// More information: https://github.com/search?q=repo%3Aarendst%2FTasmota%20esp32s2&type=code

#if defined(CONFIG_IDF_TARGET_ESP32S2)
	String BOARD_NAME = "ESP32-S2";
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
	String BOARD_NAME = "ESP32-S3";
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
	String BOARD_NAME = "ESP32-C3";
#elif defined(CONFIG_IDF_TARGET_ESP32)
	String BOARD_NAME = "ESP32";
#endif

	return BOARD_NAME;
}

String get_mac_address()
{
	uint8_t mac[6];

	// Put the addr in mac
	WiFi.macAddress(mac);

	// Build a string and return it
	char buf[20] = "";
	snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	String ret = buf;

	return ret;
}

#ifdef ESP32
int8_t check_auth(WebServer *server)
{
#endif
	// If we have a user and a password we check digest auth
	bool use_auth = (strlen(WWW_USER) && strlen(WWW_PASSWORD));
	if (!use_auth)
	{
		return 1;
	}

	if (!server->authenticate(WWW_USER, WWW_PASSWORD))
	{
		// Basic Auth Method
		// return server.requestAuthentication(BASIC_AUTH, WWW_REALM, authFailResponse);

		// Digest Auth
		server->requestAuthentication(DIGEST_AUTH, WWW_REALM, authFailResponse);

		return 0;
	}

	return 2;
}

#ifdef ESP32
int WebOTA::add_http_routes(WebServer *server, const char *path)
{
#endif
	// Index page
	server->on("/", HTTP_GET, [server]()
			   {
    check_auth(server);
    server->sendHeader("Location", "/update");
    server->send(302, "text/plain", ""); });

	// Upload firmware page
	server->on(path, HTTP_GET, [server, this]()
			   {
		check_auth(server);

		String html = "";
			uint32_t maxSketchSpace = this->max_sketch_size();
			String uptime_str = human_time(millis() / 1000);
			String board_type = webota.get_board_type();
			String mac_addr   = get_mac_address();

			char buf[1024];
			snprintf_P(buf, sizeof(buf), INDEX_HTML, (String)maxSketchSpace, board_type, mac_addr.c_str(), uptime_str.c_str());

			html = buf;

		server->send_P(200, "text/html", html.c_str()); });

	// Handling uploading firmware file
	server->on(path, HTTP_POST, [server, this]()
			   {
		check_auth(server);

		server->send(200, "text/plain", (Update.hasError()) ? "Update: fail\n" : "Update: OK!\n");
		delay(500);
		ESP.restart(); }, [server, this]()
			   {
		HTTPUpload& upload = server->upload();

		if (upload.status == UPLOAD_FILE_START) {
			Serial.printf("Firmware update initiated: %s\r\n", upload.filename.c_str());

			//uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
			uint32_t maxSketchSpace = this->max_sketch_size();

			if (!Update.begin(maxSketchSpace)) { //start with max available size
				Update.printError(Serial);
			}
		} else if (upload.status == UPLOAD_FILE_WRITE) {
			/* flashing firmware to ESP*/
			if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
				Update.printError(Serial);
			}

			// Store the next milestone to output
			uint16_t chunk_size  = 51200;
			static uint32_t next = 51200;

			// Check if we need to output a milestone (100k 200k 300k)
			if (upload.totalSize >= next) {
				Serial.printf("%dk ", next / 1024);
				next += chunk_size;
			}
		} else if (upload.status == UPLOAD_FILE_END) {
			if (Update.end(true)) { //true to set the size to the current progress
				Serial.printf("\r\nFirmware update successful: %u bytes\r\nRebooting...\r\n", upload.totalSize);
			} else {
				Update.printError(Serial);
			}
		} });

	// FILE: main.js
	server->on("/main.js", HTTP_GET, [server]()
			   { server->send_P(200, "application/javascript", MAIN_JS); });

	// FILE: main.css
	server->on("/main.css", HTTP_GET, [server]()
			   { server->send_P(200, "text/css", MAIN_CSS); });

	// FILE: favicon.ico
	server->on("/favicon.ico", HTTP_GET, [server]()
			   { server->send(200, "image/vnd.microsoft.icon", ""); });

	server->begin();

	return 1;
}

// If the MCU is in a delay() it cannot respond to HTTP OTA requests
// We do a "fake" looping delay and listen for incoming HTTP requests while waiting
void WebOTA::delay(unsigned int ms)
{
	// Borrowed from mshoe007 @ https://github.com/scottchiefbaker/ESP-WebOTA/issues/8
	decltype(millis()) last = millis();

	while ((millis() - last) < ms)
	{
		OTAServer.handleClient();
		::delay(5);
	}
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

int init_mdns(const char *host)
{
	// Use mdns for host name resolution
	if (!MDNS.begin(host))
	{
		Serial.println("Error setting up MDNS responder!");

		return 0;
	}

	Serial.printf("mDNS started : %s.local\r\n", host);

	webota.mdns = host;

	return 1;
}

String WebOTA::human_time(uint32_t sec)
{
	int days = (sec / 86400);
	sec = sec % 86400;
	int hours = (sec / 3600);
	sec = sec % 3600;
	int mins = (sec / 60);
	sec = sec % 60;

	char buf[24] = "";
	if (days)
	{
		snprintf(buf, sizeof(buf), "%d days %d hours\n", days, hours);
	}
	else if (hours)
	{
		snprintf(buf, sizeof(buf), "%d hours %d minutes\n", hours, mins);
	}
	else
	{
		snprintf(buf, sizeof(buf), "%d minutes %d seconds\n", mins, sec);
	}

	String ret = buf;

	return ret;
}
