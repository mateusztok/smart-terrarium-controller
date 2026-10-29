import { useCallback, useEffect, useState } from 'react';
import { Line } from 'react-chartjs-2';
import {
  CategoryScale,
  Chart as ChartJS,
  Legend,
  LinearScale,
  LineElement,
  PointElement,
  Title,
  Tooltip,
} from 'chart.js';
import { fetchDashboardData, updateSetpoints } from './api.js';
import './App.css';

ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
);

const EMPTY_VALUES = {
  temperature: 0,
  humidity: 0,
  lightIntensity: 0,
};

const EMPTY_SETPOINT_INPUTS = {
  temperature: '',
  humidity: '',
  lightIntensity: '',
};

function createChartData(label, measurements) {
  return {
    labels: measurements.map(({ timestamp }) => timestamp),
    datasets: [
      {
        label,
        data: measurements.map(({ value }) => value),
        fill: false,
        backgroundColor: 'rgb(75, 192, 192)',
        borderColor: 'rgba(75, 192, 192, 0.6)',
      },
    ],
  };
}

export default function Dashboard() {
  const [currentValues, setCurrentValues] = useState(EMPTY_VALUES);
  const [setpoints, setSetpoints] = useState(EMPTY_VALUES);
  const [newSetpoints, setNewSetpoints] = useState(EMPTY_SETPOINT_INPUTS);
  const [series, setSeries] = useState({
    temperature: [],
    humidity: [],
    light: [],
    output: [],
  });
  const [error, setError] = useState('');
  const [isSaving, setIsSaving] = useState(false);

  const refreshDashboard = useCallback(async () => {
    try {
      const data = await fetchDashboardData();
      setCurrentValues(data.currentValues);
      setSetpoints(data.setpoints);
      setSeries(data.series);
      setError('');
    } catch (requestError) {
      console.error('Unable to load dashboard data:', requestError);
      setError(`Unable to load dashboard data: ${requestError.message}`);
    }
  }, []);

  useEffect(() => {
    refreshDashboard();
    const intervalId = window.setInterval(refreshDashboard, 10_000);

    return () => window.clearInterval(intervalId);
  }, [refreshDashboard]);

  function handleInputChange(event) {
    const { name, value } = event.target;
    setNewSetpoints((previous) => ({ ...previous, [name]: value }));
  }

  async function handleSubmit(event) {
    event.preventDefault();
    setIsSaving(true);

    try {
      await updateSetpoints(newSetpoints);
      setNewSetpoints(EMPTY_SETPOINT_INPUTS);
      await refreshDashboard();
    } catch (requestError) {
      console.error('Unable to update setpoints:', requestError);
      setError(`Unable to update setpoints: ${requestError.message}`);
    } finally {
      setIsSaving(false);
    }
  }

  const measurementRows = [
    {
      label: 'Temperature',
      unit: '°C',
      value: currentValues.temperature.toFixed(2),
      setpoint: Math.round(setpoints.temperature),
      name: 'temperature',
      inputValue: newSetpoints.temperature,
    },
    {
      label: 'Humidity',
      unit: '%',
      value: Math.round(currentValues.humidity),
      setpoint: Math.round(setpoints.humidity),
      name: 'humidity',
      inputValue: newSetpoints.humidity,
    },
    {
      label: 'Light',
      unit: 'lux',
      value: Math.round(currentValues.lightIntensity),
      setpoint: Math.round(setpoints.lightIntensity),
      name: 'lightIntensity',
      inputValue: newSetpoints.lightIntensity,
    },
  ];

  const charts = [
    { title: 'Temperature', seriesName: 'temperature' },
    { title: 'Output', seriesName: 'output' },
    { title: 'Humidity', seriesName: 'humidity' },
    { title: 'Light intensity', seriesName: 'light' },
  ];

  return (
    <main className="App">
      <h1>Terrarium Environment Dashboard</h1>

      <form onSubmit={handleSubmit}>
        <div className="table-container">
          <table>
            <thead>
              <tr>
                <th>Measurement</th>
                <th>Current value</th>
                <th>Current setpoint</th>
                <th>New setpoint</th>
              </tr>
            </thead>
            <tbody>
              {measurementRows.map((measurement) => (
                <tr key={measurement.name}>
                  <td>
                    {measurement.label} ({measurement.unit})
                  </td>
                  <td>{measurement.value}</td>
                  <td>{measurement.setpoint}</td>
                  <td>
                    <input
                      aria-label={`New ${measurement.label.toLowerCase()} setpoint`}
                      name={measurement.name}
                      type="number"
                      value={measurement.inputValue}
                      onChange={handleInputChange}
                    />
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
        <button disabled={isSaving} type="submit">
          {isSaving ? 'Saving…' : 'Update setpoints'}
        </button>
      </form>

      <p aria-live="polite" className="status-message">
        {error}
      </p>

      <div className="charts">
        {charts.map(({ title, seriesName }) => (
          <section className="chart-container" key={seriesName}>
            <h2>{title}</h2>
            <Line data={createChartData(title, series[seriesName])} />
          </section>
        ))}
      </div>
    </main>
  );
}
