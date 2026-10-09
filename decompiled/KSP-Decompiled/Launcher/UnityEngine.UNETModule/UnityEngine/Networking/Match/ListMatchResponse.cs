using System;
using System.Collections.Generic;

namespace UnityEngine.Networking.Match;

[Serializable]
internal class ListMatchResponse : BasicResponse
{
	public List<MatchDesc> matches;

	public ListMatchResponse()
	{
		matches = new List<MatchDesc>();
	}

	public ListMatchResponse(List<MatchDesc> otherMatches)
	{
		matches = otherMatches;
	}

	public override string ToString()
	{
		return UnityString.Format("[{0}]-matches.Count:{1}", base.ToString(), (matches != null) ? matches.Count : 0);
	}
}
