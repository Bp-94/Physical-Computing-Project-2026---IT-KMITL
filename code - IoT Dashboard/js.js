function updateWeatherImage(isRaining) {
    const weatherImage = document.getElementById("weather-icon");
    const status = document.getElementById("weather-status");

    weatherImage.style.display = "none";
    status.innerText = "Loading...";

    if (isRaining === "RAIN") {
        weatherImage.style.display = "block";
        weatherImage.src = "raining.webp";
        status.innerText = "Raining!";
    } else if (isRaining === "DRY") {
        weatherImage.style.display = "block";
        weatherImage.src = "cloud.webp";
        status.innerText = "Mostly clear!";
    }
}

updateWeatherImage("loading");

// arduino -> web
const status1 = document.getElementById("openStatus");
const toggle = document.getElementById("status-toggle");


function updateOpenStatus(status) {

    if (status === "ON") {
        status1.innerText = "Opened";
        toggle.checked = true;
    } else if (status === "OFF") {
        status1.innerText = "Closed";
        toggle.checked = false;

    } else {
        status1.innerText = "Closed";
        toggle.checked = false;
    }
}

updateOpenStatus("loading");

// web -> arduino
// กรณีเปิดออโต้ ท้อกเกิลต้องเขียวอัตโนมัติด้วย
const client = mqtt.connect("wss://broker.hivemq.com:8884/mqtt");

const TOPIC = "projectPhyCom4Kon";
const connectText = document.getElementById("connecting_text")

// ************************************
function setConnectionStatus(text, color) {
    connectText.textContent = text;
    connectText.style.color = color;
}

client.on("connect", () => {
    console.log("MQTT connected!");
    client.publish("STATUS");
    setConnectionStatus("Connected", "#02e715");
    client.subscribe(TOPIC, (err) => {
        if (err) {
            console.error("Subscribe ไม่สำเร็จ:", err);
            return;
        }
        client.publish(TOPIC, "STATUS");
    });
});

client.on("error", (err) => {
    console.error("MQTT error! :", err);
    setConnectionStatus("Disconnected", "#e70202");
});

client.on("close", () => setConnectionStatus("Disconnected", "#e70202"));
client.on("offline", () => setConnectionStatus("Disconnected", "#e70202"));

client.on("reconnect", () => setConnectionStatus("Reconnecting...", "#f59e0b"));
// ************************************

toggle.addEventListener("change", () => {
    let msg;
    if (toggle.checked) {
        msg = "90";

    } else {
        msg = "0";
    }
    
    const options = {
        qos: 1,
        retain: false
    }

    client.publish(TOPIC, msg);

});

client.on("message", (topic, payload) => {
    if (topic === TOPIC) {
        if (payload.toString() === "ON" || payload.toString() === "OFF") {
            updateOpenStatus(payload.toString());
        } else if (payload.toString() === "RAIN" || payload.toString() === "DRY") {
            updateWeatherImage(payload.toString());
        }
    }
});
