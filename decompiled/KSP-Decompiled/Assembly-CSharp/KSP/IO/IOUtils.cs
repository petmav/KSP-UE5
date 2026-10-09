using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.Serialization.Formatters.Binary;
using UnityEngine;

namespace KSP.IO
{
	public class IOUtils
	{
		internal static string PluginRootPath
		{
			get
			{
				if (Application.platform != RuntimePlatform.OSXPlayer)
				{
					return Application.dataPath + "/../PluginData/";
				}
				return Application.dataPath + "/../../PluginData/";
			}
		}

		public static string GetFilePathFor(Type T, string file, Vessel flight = null)
		{
			string text = Path.Combine(PluginRootPath, AssemblyLoader.GetPathByType(T));
			if (flight != null)
			{
				text = Path.Combine(Path.Combine(text, "Flights"), flight.id.ToString());
				Directory.CreateDirectory(text);
			}
			file = Path.GetFileName(file);
			return Path.Combine(text, file);
		}

		internal static void Cleanup(List<Guid> validFlightIDs)
		{
			foreach (DirectoryInfo flightDirectory in GetFlightDirectories(validFlightIDs.ToArray(), include: false))
			{
				Directory.Delete(flightDirectory.FullName, recursive: true);
			}
		}

		internal static void RemoveFlightData(Guid flightID)
		{
			foreach (DirectoryInfo flightDirectory in GetFlightDirectories(new Guid[1] { flightID }, include: true))
			{
				Directory.Delete(flightDirectory.FullName, recursive: true);
			}
		}

		private static List<DirectoryInfo> GetFlightDirectories(Guid[] flightID, bool include)
		{
			List<DirectoryInfo> list = new List<DirectoryInfo>();
			List<string> list2 = new List<string>(flightID.Length);
			for (int i = 0; i < flightID.Length; i++)
			{
				list2.Add(flightID[i].ToString());
			}
			DirectoryInfo directoryInfo = new DirectoryInfo(PluginRootPath);
			if (!Directory.Exists(PluginRootPath))
			{
				Directory.CreateDirectory(PluginRootPath);
			}
			DirectoryInfo[] directories = directoryInfo.GetDirectories();
			for (int j = 0; j < directories.Length; j++)
			{
				DirectoryInfo[] directories2 = directories[j].GetDirectories();
				foreach (DirectoryInfo directoryInfo2 in directories2)
				{
					if (!(directoryInfo2.Name == "Flights"))
					{
						continue;
					}
					DirectoryInfo[] directories3 = directoryInfo2.GetDirectories();
					foreach (DirectoryInfo directoryInfo3 in directories3)
					{
						if ((include && list2.Contains(directoryInfo3.Name)) || (!include && !list2.Contains(directoryInfo3.Name)))
						{
							list.Add(directoryInfo3);
							break;
						}
					}
					break;
				}
			}
			return list;
		}

		internal static System.IO.FileMode ConvertMode(FileMode mode)
		{
			switch (mode)
			{
			default:
				return System.IO.FileMode.OpenOrCreate;
			case FileMode.CreateNew:
				return System.IO.FileMode.CreateNew;
			case FileMode.Create:
				return System.IO.FileMode.Create;
			case FileMode.Open:
				return System.IO.FileMode.Open;
			case FileMode.OpenOrCreate:
				return System.IO.FileMode.OpenOrCreate;
			case FileMode.Truncate:
				return System.IO.FileMode.Truncate;
			case FileMode.Append:
				return System.IO.FileMode.Append;
			}
		}

		internal static System.IO.SeekOrigin ConvertSeek(SeekOrigin origin)
		{
			switch (origin)
			{
			default:
				return System.IO.SeekOrigin.Current;
			case SeekOrigin.Begin:
				return System.IO.SeekOrigin.Begin;
			case SeekOrigin.Current:
				return System.IO.SeekOrigin.Current;
			case SeekOrigin.End:
				return System.IO.SeekOrigin.End;
			}
		}

		public static byte[] SerializeToBinary(object something)
		{
			using (System.IO.MemoryStream memoryStream = new System.IO.MemoryStream())
			{
				new BinaryFormatter().Serialize(memoryStream, something);
				return memoryStream.ToArray();
			}
		}

		public static object DeserializeFromBinary(byte[] input)
		{
			return new BinaryFormatter().Deserialize(new System.IO.MemoryStream(input));
		}

		internal static Exception WrapException(Exception e)
		{
			if (e is IOException)
			{
				return new IOException(e.Message, e.Source, e.StackTrace);
			}
			return e;
		}

		internal static System.IO.FileAccess ConvertAccess(FileAccess access)
		{
			return (System.IO.FileAccess)access;
		}

		internal static System.IO.FileShare ConvertShare(FileShare share)
		{
			return (System.IO.FileShare)share;
		}

		internal static string GetPartPathFor(Part part, string file)
		{
			string partPath = part.partInfo.partPath;
			file = Path.GetFileName(file);
			return Path.Combine(partPath, file);
		}
	}
}
