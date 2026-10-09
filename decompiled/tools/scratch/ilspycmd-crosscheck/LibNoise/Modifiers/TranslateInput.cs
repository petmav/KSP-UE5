using System;
using UnityEngine;

namespace LibNoise.Modifiers
{
	public class TranslateInput : IModule
	{
		public double X { get; set; }

		public double Y { get; set; }

		public double Z { get; set; }

		public IModule SourceModule { get; set; }

		public TranslateInput(IModule sourceModule, double x, double y, double z)
		{
			if (sourceModule == null)
			{
				throw new ArgumentNullException("A source module must be provided.");
			}
			SourceModule = sourceModule;
			X = x;
			Y = y;
			Z = z;
		}

		public double GetValue(Vector3d coordinate)
		{
			return GetValue(coordinate.x, coordinate.y, coordinate.z);
		}

		public double GetValue(Vector3 coordinate)
		{
			return GetValue(coordinate.x, coordinate.y, coordinate.z);
		}

		public double GetValue(double x, double y, double z)
		{
			if (SourceModule == null)
			{
				throw new NullReferenceException("A source module must be provided.");
			}
			return SourceModule.GetValue(x + X, y + Y, z + Z);
		}
	}
}
