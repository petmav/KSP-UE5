using System;
using System.Collections.Generic;

public class EditorPartListFilter<T>
{
	private Func<T, bool> filterCriteria;

	private string id;

	public string criteriaFailMessage = "";

	public Func<T, bool> FilterCriteria => filterCriteria;

	public string ID => id;

	public EditorPartListFilter(string filterID, Func<T, bool> criteria, string failMessage = "")
	{
		id = filterID;
		filterCriteria = criteria;
		criteriaFailMessage = failMessage;
	}

	public override string ToString()
	{
		return "EditorPartListFilter(" + typeof(T).Name + "): " + id + " '" + criteriaFailMessage + "'";
	}

	public static void FilterList(List<T> parts, Func<T, bool> filter)
	{
		int count = parts.Count;
		while (count-- > 0)
		{
			if (!filter(parts[count]))
			{
				parts.RemoveAt(count);
			}
		}
	}
}
