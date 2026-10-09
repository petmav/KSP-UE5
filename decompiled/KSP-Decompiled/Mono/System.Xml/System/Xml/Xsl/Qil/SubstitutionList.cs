using System.Collections;

namespace System.Xml.Xsl.Qil;

internal sealed class SubstitutionList
{
	private ArrayList s;

	public SubstitutionList()
	{
		s = new ArrayList(4);
	}

	public void AddSubstitutionPair(QilNode find, QilNode replace)
	{
		s.Add(find);
		s.Add(replace);
	}

	public void RemoveLastSubstitutionPair()
	{
		s.RemoveRange(s.Count - 2, 2);
	}

	public void RemoveLastNSubstitutionPairs(int n)
	{
		if (n > 0)
		{
			n *= 2;
			s.RemoveRange(s.Count - n, n);
		}
	}

	public QilNode FindReplacement(QilNode n)
	{
		for (int num = s.Count - 2; num >= 0; num -= 2)
		{
			if (s[num] == n)
			{
				return (QilNode)s[num + 1];
			}
		}
		return null;
	}
}
