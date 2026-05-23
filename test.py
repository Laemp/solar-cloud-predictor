import cv2
import numpy as np
import imutils
import math

# === ДЛЯ MQTT ===
import paho.mqtt.client as mqtt

# Подключение к локальному mosquitto-брокеру.
# loop_start() запускает фоновый поток который держит соединение живым
# и обрабатывает сетевые события — нам не надо вручную крутить mqtt-цикл.
mqtt_client = mqtt.Client()
mqtt_client.connect("localhost", 1883, 60)
mqtt_client.loop_start()

MQTT_TOPIC = "sky/state"
# === /MQTT ===



class SunDetector:

    def sun_detect(self, path):
        cap = cv2.VideoCapture(path)
        frame_number = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
        last_center = [0, 0]
        last_center_2 = [0, 0]

        shot = 0
        count = 0
        last_sent = None

        while True:
            success, frame = cap.read()
            if not success:
                break

            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            _, sun_mask = cv2.threshold(gray, 243, 255, cv2.THRESH_BINARY)
            sun_mask = cv2.morphologyEx(sun_mask, cv2.MORPH_CLOSE, np.ones((5, 5), np.uint8))

            blurred = cv2.GaussianBlur(sun_mask, (7, 7), 2)
            adg = cv2.Canny(blurred, 30, 60)

            circles = cv2.HoughCircles(adg, cv2.HOUGH_GRADIENT, dp=1.5, minDist=2000,
                                       param1=50, param2=30, minRadius=5, maxRadius=200)

            if circles is not None:
                circles = np.round(circles[0, :]).astype("int")
                for (x, y, r) in circles:
                    if math.sqrt((x - last_center[0])**2 + (y - last_center[1])**2) < 10 and math.sqrt((last_center_2[0] - last_center[0])**2 + (last_center_2[1] - last_center[1])**2) < 10:
                        cv2.circle(frame, (x, y), r, (0, 255, 0), 4)
                        count += 1
                        last_center_2 = [last_center[0], last_center[1]]
                        last_center = [x, y]
                    else:
                        last_center_2 = [last_center[0], last_center[1]]
                        last_center = [x, y]
                        count = 0
            else:
                count = 0

            cv2.imshow('Sun + White', frame)
            #cv2.imshow('Sun + Wh', adg)

            if frame_number != 1:
                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break
            else:
                cv2.waitKey(0)

            # Раз в 25 кадров принимаем решение
            if shot == 25:
                # Если солнце устойчиво видно >= 10 кадров — небо чистое
                state = "clear" if count >= 10 else "cloudy"
                print(state)

                if state != last_sent:
                    mqtt_client.publish(MQTT_TOPIC, state)
                    print(f"  -> published to MQTT topic '{MQTT_TOPIC}'")
                    last_sent = state

                shot = 0
            shot += 1

        cap.release()
        cv2.destroyAllWindows()

        mqtt_client.loop_stop()
        mqtt_client.disconnect()

        print("NON")


a = SunDetector()
a.sun_detect("SS.mp4")
