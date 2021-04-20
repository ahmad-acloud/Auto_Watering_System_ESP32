# Auto_Watering_System_ESP32
An IoT project that makes use of ESP32 hardware to create an automatic watering system and register sensors' readings .  
Bullet points : 
- Using ESP32 to collect humidity, temperature, soil moisture , water volume in bucket, and current time
- A water pump is triggered if a moisture sensor values are within a certain thershold
- Using AWS IoT core, data is sent every 5 second using MQTT protocol and to view the data , sucbscribe to the topic you created
- Using an IoT rule, sensor data is sent to DB table
- Using an IoT rule, SNS is set to send a message if water hits lower thershold.
- Using an IoT rule,  a lambda function excutes and recieve the json payload (data) and fed into a data stream off Kinesis data firehose 
- Simultaneously, data is dumped and stored into an S3 bucket which to be exported and analyzed using Amazon Quicksight platform   
 
- Part #1 - ESP32 IoT  Automatic Irrigation System Presentation
https://youtu.be/u4eIyrAqxdQ

- Part #2 - ESP32 Automatic Irrigation System Quick Demo.
https://youtu.be/yoQ9dS1zXUU
