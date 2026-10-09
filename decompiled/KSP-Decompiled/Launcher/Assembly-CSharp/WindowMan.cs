using System;
using System.Runtime.InteropServices;
using UnityEngine;

public class WindowMan : MonoBehaviour
{
	public int width;

	public int height;

	public bool borderless;

	private int cWidth;

	private int cHeight;

	private readonly string WINDOW_NAME = "KSPLauncher";

	private const int GWL_STYLE = -16;

	private const int WS_BORDER = 8388608;

	private const int WS_CAPTION = 12582912;

	private const int WS_SYSMENU = 524288;

	private const int WS_MINIMIZEBOX = 131072;

	private const int SWP_SHOWWINDOW = 64;

	[DllImport("USER32.DLL")]
	public static extern int SetWindowLong(IntPtr hWnd, int _nIndex, int dwNewLong);

	[DllImport("user32.dll", EntryPoint = "FindWindow", SetLastError = true)]
	private static extern IntPtr FindWindowByCaption(IntPtr ZeroOnly, string lpWindowName);

	[DllImport("user32.dll")]
	private static extern bool DrawMenuBar(IntPtr hWnd);

	[DllImport("user32.dll")]
	private static extern bool SetWindowPos(IntPtr hWnd, int hWndInsertAfter, int x, int y, int cx, int cy, uint wFlags);

	private void Start()
	{
		cWidth = Screen.currentResolution.width;
		cHeight = Screen.currentResolution.height;
		WindowedBorderless(960, 600);
	}

	public void WindowedBorderless(int _width, int _height)
	{
		IntPtr hWnd = FindWindowByCaption(IntPtr.Zero, WINDOW_NAME);
		SetWindowLong(hWnd, -16, 524288);
		SetWindowPos(hWnd, -2, cWidth / 2 - 480, cHeight / 2 - 300, _width, _height, 64u);
		DrawMenuBar(hWnd);
	}
}
