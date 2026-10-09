using System;

public class OrbitUtil
{
	private static CelestialBody[] p1Hierarchy = new CelestialBody[10];

	private static int p1i;

	private static CelestialBody[] p2Hierarchy = new CelestialBody[10];

	private static int p2i;

	public static double CurrentPhaseAngle(Orbit origin, Orbit destination)
	{
		Vector3d normalized = origin.GetOrbitNormal().normalized;
		Vector3d vector3d = Vector3d.Exclude(normalized, destination.pos);
		double num = Vector3d.Angle(origin.pos, vector3d);
		if (Vector3d.Dot(Vector3d.Cross(origin.pos, vector3d), normalized) < 0.0)
		{
			num = 360.0 - num;
		}
		if (num > 180.0)
		{
			num -= 360.0;
		}
		return num;
	}

	public static double CurrentEjectionAngle(Orbit origin, double UT)
	{
		CelestialBody referenceBody = origin.referenceBody;
		Vector3d prograde = ((referenceBody.orbit != null) ? referenceBody.orbit.getOrbitalVelocityAtUT(UT) : Vector3d.zero);
		return CurrentEjectionAngle(origin, UT, prograde);
	}

	public static double CurrentEjectionAngle(Orbit origin, double UT, Vector3d prograde)
	{
		double num = 0.0;
		Vector3d relativePositionAtUT = origin.getRelativePositionAtUT(UT);
		num = (Math.Atan2(prograde.y, prograde.x) - Math.Atan2(relativePositionAtUT.y, relativePositionAtUT.x)) * (180.0 / Math.PI) % 360.0;
		if (num > 180.0)
		{
			num -= 360.0;
		}
		if (num < -180.0)
		{
			num += 360.0;
		}
		return num;
	}

	public static IntersectInformation CalculateIntersections(Orbit referenceOrbit, Orbit targetOrbit)
	{
		IntersectInformation result = default(IntersectInformation);
		int iterationCount = 0;
		double CD = 0.0;
		double CCD = 0.0;
		double FFp = 0.0;
		double FFs = 0.0;
		double SFp = 0.0;
		double SFs = 0.0;
		result.numberOfIntersections = Orbit.FindClosestPoints(referenceOrbit, targetOrbit, ref CD, ref CCD, ref FFp, ref FFs, ref SFp, ref SFs, 0.0001, 20, ref iterationCount);
		double a = referenceOrbit.StartUT + referenceOrbit.GetDTforTrueAnomaly(FFp, 0.0);
		double b = referenceOrbit.StartUT + referenceOrbit.GetDTforTrueAnomaly(SFp, 0.0);
		if (a > b)
		{
			UtilMath.SwapValues(ref a, ref b);
		}
		Vector3d relativePositionAtUT = referenceOrbit.getRelativePositionAtUT(a);
		Vector3d relativePositionAtUT2 = targetOrbit.getRelativePositionAtUT(a);
		double magnitude = (relativePositionAtUT - relativePositionAtUT2).magnitude;
		result.intersect1Distance = magnitude;
		result.intersect1UT = a;
		if (result.numberOfIntersections > 1)
		{
			Vector3d relativePositionAtUT3 = referenceOrbit.getRelativePositionAtUT(b);
			relativePositionAtUT2 = targetOrbit.getRelativePositionAtUT(b);
			magnitude = (relativePositionAtUT3 - relativePositionAtUT2).magnitude;
			result.intersect2Distance = magnitude;
			result.intersect2UT = b;
		}
		return result;
	}

	public static double GetTransferTime(Orbit o1, Orbit o2)
	{
		return Math.PI * Math.Sqrt(Math.Pow(o1.radius + o2.radius, 3.0) / (8.0 * o1.referenceBody.gravParameter));
	}

	public static double GetTransferPhaseAngle(Orbit o1, Orbit o2, double Th)
	{
		double num = Math.Sqrt(o2.referenceBody.gravParameter / Math.Pow(o2.radius, 3.0));
		return Math.PI - num * Th;
	}

	public static CelestialBody FindPlanet(CelestialBody src)
	{
		if (src.referenceBody == null)
		{
			return null;
		}
		CelestialBody celestialBody = src;
		do
		{
			celestialBody = src;
			src = src.referenceBody;
		}
		while (src.referenceBody != src && src.referenceBody != null);
		return celestialBody;
	}

	public static CelestialBody FindCommonAncestor(Orbit o, CelestialBody cb)
	{
		if (o.referenceBody == cb)
		{
			return cb;
		}
		p1i = 0;
		GetBodyHierarchy(o.referenceBody, p1Hierarchy, ref p1i);
		p2i = 0;
		GetBodyHierarchy(cb, p2Hierarchy, ref p2i);
		CelestialBody result = null;
		while (p1i >= 0 && p2i >= 0 && p1Hierarchy[p1i] == p2Hierarchy[p2i])
		{
			result = p1Hierarchy[p1i];
			p1i--;
			p2i--;
		}
		return result;
	}

	private static void GetBodyHierarchy(CelestialBody cb, CelestialBody[] hierarchy, ref int i)
	{
		hierarchy[i] = cb;
		if (cb.referenceBody != null && cb.referenceBody != cb)
		{
			i++;
			GetBodyHierarchy(cb.referenceBody, hierarchy, ref i);
		}
	}

	public static double GetSmaDistance(Orbit o1, Orbit o2)
	{
		return GetSmaDistanceToSun(o1) - GetSmaDistanceToSun(o2);
	}

	public static double GetSmaDistanceToSun(Orbit obt, double d = 0.0)
	{
		if (obt != null)
		{
			d += obt.semiMajorAxis;
			if (obt.referenceBody != null)
			{
				d += GetSmaDistanceToSun(obt.referenceBody.orbit, d);
			}
		}
		return d;
	}
}
