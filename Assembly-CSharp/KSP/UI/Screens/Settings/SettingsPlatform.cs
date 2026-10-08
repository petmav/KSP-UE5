using System;
using UnityEngine;

namespace KSP.UI.Screens.Settings
{
	public static class SettingsPlatform
	{
		[Flags]
		public enum Platform
		{
			Everything = -1,
			None = 0,
			PC = 1,
			Linux = 2,
			OSX = 4,
			PS4 = 0x10,
			XBoxOne = 0x20,
			XBox360 = 0x40,
			WiiU = 0x80
		}

		public static Platform platformEmulator = Platform.Everything;

		public static bool IsPlatform(Platform platform)
		{
			if (Application.platform == RuntimePlatform.WindowsPlayer && (platform & Platform.PC) != 0)
			{
				return true;
			}
			if (Application.platform == RuntimePlatform.LinuxPlayer && (platform & Platform.Linux) != 0)
			{
				return true;
			}
			if (Application.platform == RuntimePlatform.OSXPlayer && (platform & Platform.OSX) != 0)
			{
				return true;
			}
			if (Application.platform == RuntimePlatform.PS4 && (platform & Platform.PS4) != 0)
			{
				return true;
			}
			if (Application.platform == RuntimePlatform.XboxOne && (platform & Platform.XBoxOne) != 0)
			{
				return true;
			}
			if ((platformEmulator & platform) != 0)
			{
				return true;
			}
			if (Application.platform != RuntimePlatform.WindowsEditor && Application.platform != 0)
			{
				return false;
			}
			return true;
		}
	}
}
