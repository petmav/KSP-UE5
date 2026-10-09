using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using UnityEngine;

namespace Lingoona
{
	public class Grammar
	{
		private static bool available;

		static Grammar()
		{
			try
			{
				NativeMethods.initialize();
				available = NativeMethods.isAvailable();
				if (!available)
				{
					Debug.Log("The Lingoona Grammar Library is no longer available. Did your trial expire?");
				}
			}
			catch (DllNotFoundException)
			{
				Debug.Log("The Lingoona Grammar Library couldn't be found.");
			}
			catch (EntryPointNotFoundException)
			{
				Debug.Log("The Lingoona Grammar Library is missing one or more entry points.");
			}
		}

		public static void setLanguage(string language)
		{
			if (available)
			{
				NativeMethods.setLanguage(language);
			}
		}

		public static string getLanguage()
		{
			if (available)
			{
				return NativeMethods.getLanguage();
			}
			return null;
		}

		public static string useGrammar(string template, List<string> parameters)
		{
			if (available)
			{
				return NativeMethods.useGrammar(template, 4096, parameters.ToArray(), parameters.Count);
			}
			Regex regex = new Regex("<<(?:[^>]+:)?([a-z_]+|\\d+)>>");
			Match match;
			while ((match = regex.Match(template)).Success)
			{
				char c = match.Groups[1].Value[0];
				if (c >= '0' && c <= '9')
				{
					int num = int.Parse(match.Groups[1].Value);
					template = template.Remove(match.Groups[0].Index, match.Groups[0].Length);
					if (num > 0 && num <= parameters.Count)
					{
						string text = parameters[num - 1];
						int num2 = text.IndexOf('^');
						if (num2 != -1)
						{
							text = text.Remove(num2);
						}
						template = template.Insert(match.Groups[0].Index, text);
					}
				}
				else
				{
					template = template.Remove(match.Groups[0].Index, 2).Remove(match.Groups[0].Index + match.Groups[0].Length - 4, 2);
				}
			}
			return template;
		}

		public static string useGrammar(string template)
		{
			if (available)
			{
				return NativeMethods.useGrammar(template, 4096, null, 0);
			}
			Regex regex = new Regex("<<(?:[^>]+:)?([a-z_]+|\\d+)>>");
			Match match;
			while ((match = regex.Match(template)).Success)
			{
				char c = match.Groups[1].Value[0];
				template = ((c < '0' || c > '9') ? template.Remove(match.Groups[0].Index, 2).Remove(match.Groups[0].Index + match.Groups[0].Length - 4, 2) : template.Remove(match.Groups[0].Index, match.Groups[0].Length));
			}
			return template;
		}

		public static string generateName(string noun, List<string> components)
		{
			if (available)
			{
				return NativeMethods.generateName(noun, components.Count, components.ToArray(), components.Count);
			}
			return noun;
		}

		public static void setVariable(string name, string value)
		{
			if (available)
			{
				NativeMethods.setVariable(name, value);
			}
		}

		public static void clearVariable(string name)
		{
			if (available)
			{
				NativeMethods.clearVariable(name);
			}
		}

		public static void clearVariables()
		{
			if (available)
			{
				NativeMethods.clearVariables();
			}
		}
	}
}
