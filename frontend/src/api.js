const API_BASE_URL = import.meta.env.VITE_API_BASE_URL ?? '/api';

async function requestText(path, options) {
  const response = await fetch(`${API_BASE_URL}${path}`, options);

  if (!response.ok) {
    throw new Error(`Request failed (${response.status} ${response.statusText})`);
  }

  return response.text();
}

function readNumber(text, pattern) {
  const match = text.match(pattern);
  return match ? Number.parseFloat(match[1]) : 0;
}

function parseRecentMeasurements(text) {
  return {
    temperature: readNumber(
      text,
      /Temperature\s*-\s*Timestamp:.*?Value:\s*(-?[\d.]+)\s*°C/i,
    ),
    humidity: readNumber(
      text,
      /Humidity\s*-\s*Timestamp:.*?Value:\s*(-?[\d.]+)\s*%/i,
    ),
    lightIntensity: readNumber(
      text,
      /Light\s*-\s*Timestamp:.*?Value:\s*(-?[\d.]+)\s*lux/i,
    ),
  };
}

function parseSetpoints(text) {
  return {
    temperature: readNumber(text, /temperature:\s*(-?[\d.]+)/i),
    humidity: readNumber(text, /humidity:\s*(-?[\d.]+)/i),
    lightIntensity: readNumber(text, /light:\s*(-?[\d.]+)/i),
  };
}

function parseSeries(text) {
  return text
    .split('\n')
    .filter((line) => line.trim() !== '')
    .map((line) => ({
      timestamp: line.match(/Timestamp:\s*([\d-]+\s[\d:]+)/)?.[1] ?? '',
      value: readNumber(line, /Value:\s*(-?[\d.]+)/),
    }))
    .reverse();
}

export async function fetchDashboardData() {
  const [
    recentMeasurements,
    setpoints,
    temperature,
    humidity,
    light,
    output,
  ] = await Promise.all([
    requestText('/recentMeasurements'),
    requestText('/getSetpoints'),
    requestText('/temperature'),
    requestText('/humidity'),
    requestText('/light'),
    requestText('/getOutput'),
  ]);

  return {
    currentValues: parseRecentMeasurements(recentMeasurements),
    setpoints: parseSetpoints(setpoints),
    series: {
      temperature: parseSeries(temperature),
      humidity: parseSeries(humidity),
      light: parseSeries(light),
      output: parseSeries(output),
    },
  };
}

export function updateSetpoints(values) {
  const body = new URLSearchParams({
    temperature: values.temperature,
    humidity: values.humidity,
    light: values.lightIntensity,
  });

  return requestText('/setSetpoints', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/x-www-form-urlencoded',
    },
    body,
  });
}
