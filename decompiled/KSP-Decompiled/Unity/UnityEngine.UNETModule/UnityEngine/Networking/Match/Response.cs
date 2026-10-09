using System;

namespace UnityEngine.Networking.Match;

[Serializable]
internal abstract class Response : IResponse
{
	public bool success;

	public string extendedInfo;

	public void SetSuccess()
	{
		success = true;
		extendedInfo = "";
	}

	public void SetFailure(string info)
	{
		success = false;
		extendedInfo += info;
	}

	public override string ToString()
	{
		return UnityString.Format("[{0}]-success:{1}-extendedInfo:{2}", base.ToString(), success, extendedInfo);
	}
}
