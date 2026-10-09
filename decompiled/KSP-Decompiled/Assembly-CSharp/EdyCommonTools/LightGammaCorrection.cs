using UnityEngine;

namespace EdyCommonTools
{
	[RequireComponent(typeof(Light))]
	public class LightGammaCorrection : MonoBehaviour
	{
		public ColorSpace intendedColorSpace = ColorSpace.Linear;

		[Range(0.5f, 1.5f)]
		public float extraCorrectionFactor = 0.8f;

		private Light m_light;

		private float m_originalIntensity;

		private Color m_originalColor;

		private void OnEnable()
		{
			m_light = GetComponent<Light>();
			m_originalIntensity = m_light.intensity;
			m_originalColor = m_light.color;
			if (intendedColorSpace == ColorSpace.Linear && QualitySettings.activeColorSpace == ColorSpace.Gamma)
			{
				m_light.intensity = LinearToGamma(m_light.intensity) * extraCorrectionFactor;
				m_light.color = m_light.color.linear * extraCorrectionFactor;
			}
			else if (intendedColorSpace == ColorSpace.Gamma && QualitySettings.activeColorSpace == ColorSpace.Linear)
			{
				m_light.intensity = GammaToLinear(m_light.intensity / extraCorrectionFactor);
				m_light.color = (m_light.color / extraCorrectionFactor).gamma;
			}
		}

		private void OnDisable()
		{
			m_light.intensity = m_originalIntensity;
			m_light.color = m_originalColor;
		}

		private float LinearToGamma(float l)
		{
			return Mathf.Pow(l, 0.45454544f);
		}

		private float GammaToLinear(float g)
		{
			return Mathf.Pow(g, 2.2f);
		}
	}
}
