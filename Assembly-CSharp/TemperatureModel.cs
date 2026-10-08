using System.Collections.Generic;

internal class TemperatureModel
{
	private enum TemperatureModels
	{
		LINEAR,
		QUADRATIC,
		CONSTANT,
		STEPPED
	}

	private class TemperatureModelTemplate
	{
		public virtual float getTemperatureAtAltitude(float altitude)
		{
			return 0f;
		}
	}

	private class LinearTemperatureModel : TemperatureModelTemplate
	{
		private float m;

		private float b;

		public LinearTemperatureModel(float m, float b)
		{
			this.m = m;
			this.b = b;
		}

		public LinearTemperatureModel(float temp1, float alt1, float temp2, float alt2)
		{
			m = (temp1 - temp2) / (alt1 - alt2);
			b = temp1 - m * alt1;
		}

		public override float getTemperatureAtAltitude(float altitude)
		{
			return m * altitude + b;
		}
	}

	private class QuadraticTemperatureModel : TemperatureModelTemplate
	{
		private float A;

		private float B;

		private float C;

		public QuadraticTemperatureModel(float A, float B, float C)
		{
			this.A = A;
			this.B = B;
			this.C = C;
		}

		public override float getTemperatureAtAltitude(float altitude)
		{
			return A * altitude * altitude + B * altitude + C;
		}
	}

	private class ConstantTemperatureModel : TemperatureModelTemplate
	{
		private float K;

		public ConstantTemperatureModel(float k)
		{
			K = k;
		}

		public override float getTemperatureAtAltitude(float altitude)
		{
			return K;
		}
	}

	private class SteppedTemperatureModel : TemperatureModelTemplate
	{
		private struct TempRange
		{
			public float minAlt;

			public float maxAlt;

			public TemperatureModels modelName;

			public TemperatureModelTemplate modelData;
		}

		private List<TempRange> Steps;

		public SteppedTemperatureModel()
		{
			Steps = new List<TempRange>();
		}

		public bool AddInitialLinearStepEquation(float min, float max, float m, float b)
		{
			if (Steps.Count > 0)
			{
				return false;
			}
			TempRange item = default(TempRange);
			item.minAlt = min;
			item.maxAlt = max;
			item.modelName = TemperatureModels.LINEAR;
			item.modelData = new LinearTemperatureModel(m, b);
			Steps.Add(item);
			return true;
		}

		public bool AddInitialLinearStep(float temp1, float alt1, float temp2, float alt2)
		{
			if (Steps.Count > 0)
			{
				return false;
			}
			TempRange item = default(TempRange);
			item.minAlt = ((temp1 < temp2) ? temp1 : temp2);
			item.maxAlt = ((temp1 > temp2) ? temp1 : temp2);
			item.modelName = TemperatureModels.LINEAR;
			item.modelData = new LinearTemperatureModel(temp1, alt1, temp2, alt2);
			Steps.Add(item);
			return true;
		}

		public bool AddInitialQuadraticStep(float min, float max, float A, float B, float C)
		{
			if (Steps.Count > 0)
			{
				return false;
			}
			TempRange item = default(TempRange);
			item.minAlt = min;
			item.maxAlt = max;
			item.modelName = TemperatureModels.QUADRATIC;
			item.modelData = new QuadraticTemperatureModel(A, B, C);
			Steps.Add(item);
			return true;
		}

		public bool AddInitialConstantStep(float min, float max, float K)
		{
			if (Steps.Count > 0)
			{
				return false;
			}
			TempRange item = default(TempRange);
			item.minAlt = min;
			item.maxAlt = max;
			item.modelName = TemperatureModels.LINEAR;
			item.modelData = new ConstantTemperatureModel(K);
			Steps.Add(item);
			return true;
		}

		public bool AddConstantStep(float limit, float K)
		{
			if (Steps.Count == 0)
			{
				return AddInitialConstantStep((limit < 0f) ? limit : 0f, (limit < 0f) ? 0f : limit, K);
			}
			float num = ((limit < 0f) ? 0f : limit);
			int num2 = 0;
			int count = Steps.Count;
			while (true)
			{
				if (num2 < count)
				{
					if (!(num <= Steps[num2].maxAlt))
					{
						break;
					}
					num2++;
					continue;
				}
				return false;
			}
			TempRange item = default(TempRange);
			item.minAlt = Steps[num2].maxAlt;
			item.maxAlt = num;
			item.modelName = TemperatureModels.CONSTANT;
			item.modelData = new ConstantTemperatureModel(K);
			Steps.Add(item);
			return true;
		}

		public bool AddLinearStep(float limit, float temp2)
		{
			if (Steps.Count == 0)
			{
				return AddInitialLinearStep((limit < 0f) ? temp2 : 0f, (limit < 0f) ? limit : 0f, (limit < 0f) ? 0f : temp2, (limit < 0f) ? 0f : limit);
			}
			float num = ((limit < 0f) ? 0f : limit);
			int num2 = 0;
			int count = Steps.Count;
			while (true)
			{
				if (num2 < count)
				{
					if (!(num <= Steps[num2].maxAlt))
					{
						break;
					}
					num2++;
					continue;
				}
				return false;
			}
			TempRange item = default(TempRange);
			item.minAlt = Steps[num2].maxAlt;
			item.maxAlt = num;
			item.modelName = TemperatureModels.LINEAR;
			item.modelData = new LinearTemperatureModel(Steps[num2].modelData.getTemperatureAtAltitude(Steps[num2].maxAlt), Steps[num2].maxAlt, temp2, limit);
			Steps.Add(item);
			return true;
		}

		public bool AddQuadraticStep(float limit, float A, float B, float C)
		{
			if (Steps.Count == 0)
			{
				return AddInitialQuadraticStep((limit < 0f) ? limit : 0f, (limit < 0f) ? 0f : limit, A, B, C);
			}
			float num = ((limit < 0f) ? 0f : limit);
			int num2 = 0;
			int count = Steps.Count;
			while (true)
			{
				if (num2 < count)
				{
					if (!(num <= Steps[num2].maxAlt))
					{
						break;
					}
					num2++;
					continue;
				}
				return false;
			}
			TempRange item = default(TempRange);
			item.minAlt = Steps[num2].maxAlt;
			item.maxAlt = num;
			item.modelName = TemperatureModels.QUADRATIC;
			item.modelData = new QuadraticTemperatureModel(A, B, C);
			Steps.Add(item);
			return true;
		}

		public override float getTemperatureAtAltitude(float altitude)
		{
			int num = 0;
			int count = Steps.Count;
			while (true)
			{
				if (num < count)
				{
					if ((double)altitude < (double)Steps[num].maxAlt + 0.0001 && !((double)altitude <= (double)Steps[num].minAlt - 0.0001))
					{
						break;
					}
					num++;
					continue;
				}
				return 0f;
			}
			return Steps[num].modelData.getTemperatureAtAltitude(altitude);
		}

		public void clearIntervals()
		{
			Steps.Clear();
		}
	}

	private SteppedTemperatureModel Intervals;

	public TemperatureModel()
	{
		Intervals = new SteppedTemperatureModel();
	}

	public bool AddInitialConstantStep(float min, float max, float K)
	{
		return Intervals.AddInitialConstantStep(min, max, K);
	}

	public bool AddInitialLinearStep(float temp1, float alt1, float temp2, float alt2)
	{
		return Intervals.AddInitialLinearStep(temp1, alt1, temp2, alt2);
	}

	public bool AddInitialLinearStepEquation(float min, float max, float m, float b)
	{
		return Intervals.AddInitialLinearStepEquation(min, max, m, b);
	}

	public bool AddInitialQuadraticStep(float min, float max, float A, float B, float C)
	{
		return Intervals.AddInitialQuadraticStep(min, max, A, B, C);
	}

	public bool AddConstantStep(float limit, float K)
	{
		return Intervals.AddConstantStep(limit, K);
	}

	public bool AddLinearStep(float limit, float temp2)
	{
		return Intervals.AddLinearStep(limit, temp2);
	}

	public bool AddQuadraticStep(float limit, float A, float B, float C)
	{
		return Intervals.AddQuadraticStep(limit, A, B, C);
	}

	private float getTemperatureAtAltitude(double altitude)
	{
		return Intervals.getTemperatureAtAltitude((float)altitude);
	}
}
