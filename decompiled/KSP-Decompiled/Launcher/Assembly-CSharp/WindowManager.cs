using UnityEngine;

public class WindowManager : MonoBehaviour
{
	private void Awake()
	{
		Resolution[] resolutions = Screen.resolutions;
		int num = resolutions.Length;
		for (int i = 0; i < num; i++)
		{
			Debug.Log("Resolution: " + resolutions[i].ToString());
			if ((float)(resolutions[i].width / resolutions[i].height) >= 1.6f)
			{
				Screen.SetResolution(resolutions[i].width, resolutions[i].height, fullscreen: false);
				break;
			}
		}
		int num2 = num / 2;
		Screen.SetResolution(resolutions[num2 - 1].width, resolutions[num2 - 1].height, fullscreen: false);
	}
}
