using UnityEngine;

namespace EdyCommonTools
{
	public class ScaleController : MonoBehaviour
	{
		public delegate void OnScaleFinished();

		public Vector3 scale = Vector3.one;

		public bool unified = true;

		public bool damped = true;

		public float damping = 8f;

		public bool clamped;

		public Vector3 min = Vector3.one * 0.1f;

		public Vector3 max = Vector3.one * 2f;

		public OnScaleFinished onScaleFinished;

		private Transform m_trans;

		private Vector3 m_currentScale;

		private bool m_scaling;

		public void ResetScale(Vector3 newScale)
		{
			scale = newScale;
			m_currentScale = newScale;
		}

		private void OnEnable()
		{
			m_trans = GetComponent<Transform>();
			scale = m_trans.localScale;
			m_currentScale = scale;
			m_scaling = false;
		}

		public void ClampScale(ref Vector3 s)
		{
			if (clamped)
			{
				s.x = Mathf.Clamp(s.x, min.x, max.x);
				s.y = Mathf.Clamp(s.y, min.y, max.y);
				s.z = Mathf.Clamp(s.z, min.z, max.z);
			}
		}

		private void LateUpdate()
		{
			ClampScale(ref scale);
			Vector3 vector = scale;
			if (unified)
			{
				vector.y = vector.x;
				vector.z = vector.x;
			}
			if (damped)
			{
				m_currentScale = Vector3.Lerp(m_currentScale, vector, damping * Time.deltaTime);
			}
			else
			{
				if (m_currentScale != vector)
				{
					m_scaling = true;
				}
				m_currentScale = vector;
			}
			m_trans.localScale = m_currentScale;
			if ((m_currentScale - vector).magnitude > 0.001f)
			{
				m_scaling = true;
			}
			else if (m_scaling)
			{
				m_scaling = false;
				if (onScaleFinished != null)
				{
					onScaleFinished();
				}
			}
		}
	}
}
