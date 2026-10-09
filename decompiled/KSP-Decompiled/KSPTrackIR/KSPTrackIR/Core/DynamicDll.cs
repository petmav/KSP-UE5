using System;
using System.Runtime.InteropServices;

namespace KSPTrackIR.Core
{
	internal class DynamicDll
	{
		protected int handle;

		[DllImport("user32.dll")]
		public static extern int GetForegroundWindow();

		[DllImport("kernel32.dll")]
		private static extern int LoadLibrary([MarshalAs(UnmanagedType.LPStr)] string lpLibFileName);

		[DllImport("kernel32.dll")]
		private static extern IntPtr GetProcAddress(int hModule, [MarshalAs(UnmanagedType.LPStr)] string lpProcName);

		[DllImport("kernel32.dll")]
		private static extern bool FreeLibrary(int hModule);

		public DynamicDll(string dllPath)
		{
			handle = LoadLibrary(dllPath);
		}

		~DynamicDll()
		{
			FreeLibrary(handle);
		}

		public Delegate GetMethodDelegate<T>(string externMethod)
		{
			return Marshal.GetDelegateForFunctionPointer(GetProcAddress(handle, externMethod), typeof(T));
		}
	}
}
