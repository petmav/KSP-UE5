using System;
using System.IO;
using System.Runtime.InteropServices;
using KSPTrackIR.Core;
using Microsoft.Win32;
using UnityEngine;

namespace KSPTrackIR
{
	public sealed class Client
	{
		private delegate ClientResult NP_GetDataExtCall(ref TrackIRData data);

		private delegate ClientResult NP_GetParameterExtCall();

		private delegate ClientResult NP_GetSignatureExtCall(ref TrackIRSignatureData signature);

		private delegate ClientResult NP_QueryVersionExtCall(ref ushort version);

		private delegate ClientResult NP_ReCenterExtCall();

		private delegate ClientResult NP_RegisterProgramProfileIDExtCall(ushort pID);

		private delegate ClientResult NP_RegisterWindowHandleExtCall(int handle);

		private delegate ClientResult NP_RequestDataExtCall(ushort data);

		private delegate ClientResult NP_SetParameterExtCall();

		private delegate ClientResult NP_StartCursorExtCall();

		private delegate ClientResult NP_StartDataTransmissionExtCall();

		private delegate ClientResult NP_StopCursorExtCall();

		private delegate ClientResult NP_StopDataTransmissionExtCall();

		private delegate ClientResult NP_UnregisterWindowHandleExtCall();

		public enum ClientStatus
		{
			RemoteActive,
			RemoteDisabled
		}

		public enum ClientResult
		{
			Ok,
			DeviceNotPresent,
			UnsupportedOS,
			InvalidARG,
			DLLNotFound,
			NoData,
			InternalData,
			AlreadyRegistered,
			UnkownID,
			ReadOnly
		}

		public struct TrackIRSignatureData
		{
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 200)]
			public string dllSignature;

			[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 200)]
			public string appSignature;
		}

		public struct TrackIRData
		{
			public ushort status;

			public ushort frameSignature;

			public uint ioData;

			public float roll;

			public float pitch;

			public float yaw;

			public float x;

			public float y;

			public float z;

			public float rawX;

			public float rawY;

			public float rawZ;

			public float deltaX;

			public float deltaY;

			public float deltaZ;

			public float smoothX;

			public float smoothY;

			public float smoothZ;
		}

		private DynamicDll npClient;

		private NP_GetDataExtCall npGetData;

		private NP_GetParameterExtCall npGetParameter;

		private NP_GetSignatureExtCall npGetSignature;

		private NP_QueryVersionExtCall npQueryVersion;

		private NP_ReCenterExtCall npReCenter;

		private NP_RegisterProgramProfileIDExtCall npRegisterProgramProfileID;

		private NP_RegisterWindowHandleExtCall npRegisterWindowHandle;

		private NP_RequestDataExtCall npRequestData;

		private NP_SetParameterExtCall npSetParameter;

		private NP_StartCursorExtCall npStartCursor;

		private NP_StartDataTransmissionExtCall npStartDataTransmission;

		private NP_StopCursorExtCall npStopCursor;

		private NP_StopDataTransmissionExtCall npStopDataTransmission;

		private NP_UnregisterWindowHandleExtCall npUnregisterWindowHandle;

		private bool verbose;

		public bool GetData(ref TrackIRData data, out ClientResult result)
		{
			result = npGetData(ref data);
			if (result != 0)
			{
				LogError("Error retriving data!", result);
				return false;
			}
			return true;
		}

		public bool GetParameter(out ClientResult result)
		{
			result = npGetParameter();
			if (result != 0)
			{
				LogError("Error retriving parameter!", result);
				return false;
			}
			return true;
		}

		public bool GetSignature(ref TrackIRSignatureData signature, out ClientResult result)
		{
			result = npGetSignature(ref signature);
			if (result != 0)
			{
				LogError("Error retriving signature!", result);
				return false;
			}
			return true;
		}

		public bool QueryVersion(ref ushort version, out ClientResult result)
		{
			result = npQueryVersion(ref version);
			if (result != 0)
			{
				LogError("Error retriving version!", result);
				return false;
			}
			return true;
		}

		public bool ReCenter(out ClientResult result)
		{
			result = npReCenter();
			if (result != 0)
			{
				LogError("Error while re centering!", result);
				return false;
			}
			return true;
		}

		public bool RegisterProgramProfileID(ushort pID, out ClientResult result)
		{
			result = npRegisterProgramProfileID(pID);
			if (result != 0)
			{
				LogError("Error while registering program profile id!", result);
				return false;
			}
			return true;
		}

		public bool RegisterWindowHandle(int wHandle, out ClientResult result)
		{
			result = npRegisterWindowHandle(wHandle);
			if (result != 0)
			{
				LogError("Error while registering window handle ({0})!", result, wHandle);
				return false;
			}
			return true;
		}

		public bool RequestData(ushort data, out ClientResult result)
		{
			result = npRequestData(data);
			if (result != 0)
			{
				LogError("Error requesting data!", result);
				return false;
			}
			return true;
		}

		public bool SetParameter(out ClientResult result)
		{
			result = npSetParameter();
			if (result != 0)
			{
				LogError("Error setting parameter!", result);
				return false;
			}
			return true;
		}

		public bool StartCursor(out ClientResult result)
		{
			result = npStartCursor();
			if (result != 0)
			{
				LogError("Error starting cursor!", result);
				return false;
			}
			return true;
		}

		public bool StartDataTransmission(out ClientResult result)
		{
			result = npStartDataTransmission();
			if (result != 0)
			{
				LogError("Error starting data transmission!", result);
				return false;
			}
			return true;
		}

		public bool StopCursor(out ClientResult result)
		{
			result = npStopCursor();
			if (result != 0)
			{
				LogError("Error stopping cursor!", result);
				return false;
			}
			return true;
		}

		public bool StopDataTransmission(out ClientResult result)
		{
			result = npStopDataTransmission();
			if (result != 0)
			{
				LogError("Error stopping data transmission!", result);
				return false;
			}
			return true;
		}

		public bool UnregisterWindowHandle(out ClientResult result)
		{
			result = npUnregisterWindowHandle();
			if (result != 0)
			{
				LogError("Error unregistering window handle!", result);
				return false;
			}
			return true;
		}

		public Client()
			: this(null, verbose: false)
		{
		}

		public Client(string path)
			: this(path, verbose: false)
		{
		}

		public Client(bool verbose)
			: this(null, verbose)
		{
		}

		public Client(string path, bool verbose)
		{
			this.verbose = verbose;
			if (string.IsNullOrEmpty(path) || !Directory.Exists(path))
			{
				path = GetDllPath();
			}
			string text = path + GetDllName();
			if (File.Exists(text))
			{
				npClient = new DynamicDll(text);
				npGetData = (NP_GetDataExtCall)npClient.GetMethodDelegate<NP_GetDataExtCall>("NP_GetData");
				npGetParameter = (NP_GetParameterExtCall)npClient.GetMethodDelegate<NP_GetParameterExtCall>("NP_GetParameter");
				npGetSignature = (NP_GetSignatureExtCall)npClient.GetMethodDelegate<NP_GetSignatureExtCall>("NP_GetSignature");
				npQueryVersion = (NP_QueryVersionExtCall)npClient.GetMethodDelegate<NP_QueryVersionExtCall>("NP_QueryVersion");
				npReCenter = (NP_ReCenterExtCall)npClient.GetMethodDelegate<NP_ReCenterExtCall>("NP_ReCenter");
				npRegisterProgramProfileID = (NP_RegisterProgramProfileIDExtCall)npClient.GetMethodDelegate<NP_RegisterProgramProfileIDExtCall>("NP_RegisterProgramProfileID");
				npRegisterWindowHandle = (NP_RegisterWindowHandleExtCall)npClient.GetMethodDelegate<NP_RegisterWindowHandleExtCall>("NP_RegisterWindowHandle");
				npRequestData = (NP_RequestDataExtCall)npClient.GetMethodDelegate<NP_RequestDataExtCall>("NP_RequestData");
				npSetParameter = (NP_SetParameterExtCall)npClient.GetMethodDelegate<NP_SetParameterExtCall>("NP_SetParameter");
				npStartCursor = (NP_StartCursorExtCall)npClient.GetMethodDelegate<NP_StartCursorExtCall>("NP_StartCursor");
				npStartDataTransmission = (NP_StartDataTransmissionExtCall)npClient.GetMethodDelegate<NP_StartDataTransmissionExtCall>("NP_StartDataTransmission");
				npStopCursor = (NP_StopCursorExtCall)npClient.GetMethodDelegate<NP_StopCursorExtCall>("NP_StopCursor");
				npStopDataTransmission = (NP_StopDataTransmissionExtCall)npClient.GetMethodDelegate<NP_StopDataTransmissionExtCall>("NP_StopDataTransmission");
				npUnregisterWindowHandle = (NP_UnregisterWindowHandleExtCall)npClient.GetMethodDelegate<NP_UnregisterWindowHandleExtCall>("NP_UnregisterWindowHandle");
				return;
			}
			throw new FileNotFoundException("Dll not found at path " + text, text);
		}

		public bool Init()
		{
			Log("Initializing...");
			ClientResult result = ClientResult.Ok;
			TrackIRSignatureData signature = default(TrackIRSignatureData);
			if (GetSignature(ref signature, out result))
			{
				Log("{0}\n{1}", signature.appSignature, signature.dllSignature);
				int foregroundWindow = DynamicDll.GetForegroundWindow();
				if (RegisterWindowHandle(foregroundWindow, out result) && RequestData(119, out result) && RegisterProgramProfileID(20430, out result))
				{
					Log("Program profile registered!");
					if (StopCursor(out result))
					{
						Log("Cursor stopped!");
						if (StartDataTransmission(out result))
						{
							Log("Data trasmission started!");
						}
					}
				}
			}
			return result == ClientResult.Ok;
		}

		public TrackIRData GetTrackIRData()
		{
			TrackIRData data = default(TrackIRData);
			ClientResult result = ClientResult.Ok;
			GetData(ref data, out result);
			return data;
		}

		public bool Shutdown()
		{
			ClientResult result = ClientResult.Ok;
			if (StopDataTransmission(out result) && StartCursor(out result))
			{
				UnregisterWindowHandle(out result);
			}
			return result == ClientResult.Ok;
		}

		private string GetDllPath()
		{
			string text = "";
			RegistryKey registryKey = Registry.CurrentUser.OpenSubKey("Software\\NaturalPoint\\NATURALPOINT\\NPClient Location", writable: false);
			if (registryKey != null)
			{
				text = registryKey.GetValue("Path").ToString();
				registryKey.Close();
			}
			else
			{
				string text2 = Path.Combine(Environment.GetEnvironmentVariable("ProgramFiles(x86)"), "NaturalPoint\\TrackIR5\\");
				if (!Directory.Exists(text2))
				{
					text2 = Path.Combine(Environment.GetEnvironmentVariable("ProgramFiles"), "NaturalPoint\\TrackIR5\\");
				}
				text = text2;
			}
			return text;
		}

		private string GetDllName()
		{
			if (IntPtr.Size == 4)
			{
				return "NPClient.dll";
			}
			return "NPClient64.dll";
		}

		private void Log(string text, params object[] data)
		{
			if (verbose)
			{
				Debug.Log(string.Format("[KSPTrackIR]: " + text, data));
			}
		}

		private void LogError(string text, ClientResult result, params object[] data)
		{
			if (verbose && result != 0)
			{
				Debug.LogError(string.Format("[KSPTrackIR]: " + text + " (" + result.ToString() + ")", data));
			}
		}
	}
}
