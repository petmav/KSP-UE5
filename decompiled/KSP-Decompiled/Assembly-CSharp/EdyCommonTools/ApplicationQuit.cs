using UnityEngine;

namespace EdyCommonTools
{
	public class ApplicationQuit : MonoBehaviour
	{
		public bool desktop = true;

		public bool mobile = true;

		public KeyCode quitKey = KeyCode.Escape;

		private bool m_isMobile;

		private void OnEnable()
		{
			m_isMobile = Application.isMobilePlatform;
		}

		private void Update()
		{
			if (((desktop && !m_isMobile) || (mobile && m_isMobile)) && Input.GetKeyDown(quitKey))
			{
				Application.Quit();
			}
		}
	}
}
