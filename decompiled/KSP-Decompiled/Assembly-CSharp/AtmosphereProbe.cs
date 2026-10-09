using UnityEngine;

public class AtmosphereProbe : MonoBehaviour
{
	public double altitude;

	public double pressure;

	public double density;

	public double temperature;

	private void Start()
	{
	}

	private void Update()
	{
		if (FlightGlobals.ready)
		{
			altitude = FlightGlobals.getAltitudeAtPos(base.transform.position, FlightGlobals.currentMainBody);
			pressure = FlightGlobals.getStaticPressure(altitude);
			temperature = FlightGlobals.getExternalTemperature(altitude);
			density = FlightGlobals.getAtmDensity(pressure, temperature);
		}
	}
}
