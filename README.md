# IoT AWS Environmental Monitoring Project

## Descripción

Este proyecto de **Internet de las Cosas (IoT)** recopila datos ambientales en tiempo real (temperatura, humedad y calidad del aire) mediante un microcontrolador **ESP32** conectado a sensores. Los datos se envían automáticamente a **AWS**, donde se almacenan, procesan y supervisan usando servicios como **DynamoDB**, **SageMaker** y **SNS** para análisis y alertas inteligentes.  
Incluye simulaciones y análisis local en un **Jupyter Notebook**.

---

## Estructura de Archivos

- `esp32_iot.ino`  
  Código para el **ESP32**. Recopila los datos de sensores y los envía a AWS (vía API Gateway o AWS IoT Core).

- `iot_lambda.py`  
  Función **AWS Lambda**. Procesa los datos entrantes, almacena en **DynamoDB**, analiza con **SageMaker**, y lanza alertas usando **SNS** si detecta anomalías.

- `iot_jupyter.ipynb`  
  **Jupyter Notebook** para pruebas, visualización y análisis de datos localmente.

---

## Flujo General del Proyecto

1. **Recopilación de Datos (ESP32):**
    - Lee datos de sensores (temperatura, humedad, calidad de aire).
    - Envía datos formateados como JSON a un endpoint en AWS (API Gateway/IoT Core).
2. **Recepción y Procesamiento (AWS Lambda):**
    - Recibe datos del microcontrolador.
    - Valida y almacena en **DynamoDB**.
    - Prepara los datos y los manda a **SageMaker** para análisis/anomalías.
    - Si SageMaker detecta valores anómalos, dispara una alerta por **SNS** (correo/SMS/otros).
3. **Análisis y Simulación Local (Jupyter Notebook):**
    - Permite emular el flujo de datos, probar lógicas y visualizar resultados.
    - Útil para desarrollo, ajuste de modelos o exploración de datos históricos.

---

## Requisitos

### Hardware

- **ESP32** con:
  - Sensor DHT11 o DHT22 (temperatura y humedad)
  - Sensor MQ135 (calidad del aire)
- Cableado básico

### Software y Servicios

- **Arduino IDE** para cargar `esp32_iot.ino`
- **Python 3.x** y paquetes:
  - boto3, json, pandas, numpy, matplotlib, notebook
- Cuenta de **AWS** con:
  - **DynamoDB** (tabla para almacenar datos)
  - **SageMaker** (endpoint para inferencia/análisis de anomalías)
  - **SNS** (para enviar alertas)
  - **IAM**: Roles y permisos adecuados
- **Jupyter Notebook**

---

## Detalle de cada archivo

### 1. esp32_iot.ino

- Lee periódicamente sensores (temperatura, humedad, calidad de aire).
- Genera un JSON con los datos y un timestamp.
- Se conecta vía WiFi y envía los datos usando HTTP POST a AWS (API Gateway recomendado).
- El campo `estado_aire` representa un estado categórico según el valor del sensor de aire.

**Puntos a configurar en tu código:**
- SSID y password WiFi.
- Endpoint AWS (API Gateway/IoT Core).
- Mapeo correcto de pines del ESP32.

### 2. iot_lambda.py

- **Procesos principales:**
  - **Validación** de los datos entrantes (asegura que los campos clave existan).
  - **Almacenamiento**: Inserta datos en DynamoDB (temperatura, humedad, calidad de aire, estado y timestamp).
  - **Preprocesamiento**: Ordena las variables y agrega variables de tiempo antes de enviar el vector a SageMaker.
  - **Inferencia con SageMaker**: Envía el vector y recibe una puntuación de anomalía.
  - **Alertas**: Si la puntuación supera el umbral, envía alerta vía SNS.

- **Variables de entorno necesarias en AWS Lambda:**
  - `DYNAMODB_TABLE_NAME`
  - `SAGEMAKER_ENDPOINT_NAME`
  - `SNS_TOPIC_ARN`

### 3. iot_jupyter.ipynb

- Permite hacer simulaciones locales:
  - Generar o cargar datos de sensores.
  - Probar procesamiento, almacenamiento y lógica de detección de anomalías.
  - Visualizar resultados, hacer gráficos, y experimentar con los modelos de análisis.

---

## Ejecución Paso a Paso

### 1. **Configura AWS**

- **Crea una tabla en DynamoDB** con clave primaria: `timestamp`.
- **Implementa y despliega un endpoint en SageMaker** para análisis/anomalía.
- **Configura un tópico SNS** para recibir alertas.
- **Crea una función Lambda** con el contenido de `iot_lambda.py` y configura las variables de entorno indicadas.

### 2. **Carga y prueba el ESP32**

- Abre `esp32_iot.ino` en Arduino IDE.
- Ajusta SSID, password, y endpoint de AWS.
- Carga el sketch en tu ESP32.
- Monitorea el puerto serie para depuración.

### 3. **Simulación y análisis local**

- Abre `iot_jupyter.ipynb` en Jupyter Notebook.
- Simula datos o usa datos reales exportados.
- Realiza visualizaciones, explora comportamiento, o ajusta parámetros del modelo.

---

## Personalización

- Puedes ajustar el umbral de alerta en `iot_lambda.py` (`ANOMALY_SCORE_THRESHOLD`).
- El modelo de SageMaker puede ser de regresión, clasificación o un detector de anomalías. Ajusta el preprocesamiento en la función `preprocess_data_for_sagemaker()` según tu modelo.
- Para otros sensores, adapta tanto el código del ESP32 como la lógica de Lambda.

---

## Ejemplo de Payload (JSON)

```json
{
  "timestamp": "2025-07-12T21:00:00Z",
  "temperature": 27.5,
  "humidity": 68,
  "air_quality": 320,
  "estado_aire": "bueno"
}
