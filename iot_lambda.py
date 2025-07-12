import json
import boto3
import os
from datetime import datetime
from decimal import Decimal

# --- Clientes de AWS ---
dynamodb = boto3.resource('dynamodb')
sagemaker_runtime = boto3.client('sagemaker-runtime')
sns = boto3.client('sns')

# --- Configuración Leída de las Variables de Entorno ---
DYNAMODB_TABLE_NAME = os.environ['DYNAMODB_TABLE_NAME']
SAGEMAKER_ENDPOINT_NAME = os.environ['SAGEMAKER_ENDPOINT_NAME']
SNS_TOPIC_ARN = os.environ['SNS_TOPIC_ARN']

# --- Constante de Umbral ---
ANOMALY_SCORE_THRESHOLD = 1.3 # El umbral que definiste

def preprocess_data_for_sagemaker(data):
    """
    Prepara los datos para SageMaker, replicando el script de entrenamiento:
    1. Extrae características del tiempo.
    2. Elimina la información de 'estado_aire'.
    3. Ordena las características correctamente.
    """
    now = datetime.now()
    hora = now.hour
    dia_semana = now.weekday()
    mes = now.month
    
    # Ensambla el vector de características en el ORDEN EXACTO que el modelo espera.
    # Basado en tu script, el orden sería: air_quality, humidity, temperature, y luego los de tiempo.
    # ¡Revisa el orden de las columnas de tu df final si es diferente!
    feature_vector = [
        data['air_quality'],
        data['humidity'],
        data['temperature'],
        hora,
        dia_semana,
        mes
    ]
    
    return feature_vector

def lambda_handler(event, context):
    try:
        print(f"Evento recibido: {event}")
        payload = event.get('payload', event)

        if isinstance(payload, str):
            payload = json.loads(payload)

        # Valida que los campos necesarios para DynamoDB y SageMaker están presentes
        required_fields = ['timestamp', 'temperature', 'humidity', 'air_quality', 'estado_aire']
        if not all(field in payload for field in required_fields):
            print(f"Error: Faltan campos en el payload: {payload}")
            return {'statusCode': 400, 'body': f'Faltan campos'}

        # Guardar en DynamoDB (sin cambios, se guarda el dato completo)
        table = dynamodb.Table(DYNAMODB_TABLE_NAME)
        table.put_item(Item={
            'timestamp': payload['timestamp'].strip(),
            'temperature': Decimal(str(payload['temperature'])),
            'humidity': Decimal(str(payload['humidity'])),
            'air_quality_sensor_1': int(payload['air_quality']),
            'estado_aire': payload['estado_aire']
        })
        print("Datos guardados en DynamoDB.")

        # Preprocesar datos para SageMaker (usando la nueva función)
        feature_vector = preprocess_data_for_sagemaker(payload)
        print(f"Vector de características para SageMaker: {feature_vector}")
        
        # Enviar a SageMaker
        csv_payload = ','.join(map(str, feature_vector))
        response = sagemaker_runtime.invoke_endpoint(
            EndpointName=SAGEMAKER_ENDPOINT_NAME,
            ContentType='text/csv',
            Body=csv_payload
        )
        
        result_json = json.loads(response['Body'].read().decode())
        score = result_json['scores'][0]['score']
        print(f"Puntuación de anomalía recibida: {score}")

        # Disparar alerta si es necesario
        if score > ANOMALY_SCORE_THRESHOLD:
            print(f"¡ANOMALÍA DETECTADA! Puntuación: {score} > Umbral: {ANOMALY_SCORE_THRESHOLD}")
            message = (
                f"🚨 Alerta de Anomalía Detectada 🚨\n\n"
                f"Puntuación de SageMaker: {score:.4f}\n"
                f"Umbral configurado: {ANOMALY_SCORE_THRESHOLD}\n\n"
                f"Datos del Sensor:\n{json.dumps(payload, indent=2)}"
            )
            sns.publish(
                TopicArn=SNS_TOPIC_ARN,
                Message=message,
                Subject='Alerta de Anomalía en datos ambientales'
            )
            print("Alerta enviada a SNS.")
            
        return {'statusCode': 200, 'body': json.dumps({'anomaly_score': score})}

    except Exception as e:
        print(f"Error inesperado: {e}")
        return {'statusCode': 500, 'body': str(e)}