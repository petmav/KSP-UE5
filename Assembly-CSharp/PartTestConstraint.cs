using System;
using System.Collections.Generic;
using Contracts;
using UnityEngine;

public class PartTestConstraint : IConfigNode
{
	public enum ConstraintType
	{
		SPEED,
		SPEEDENV,
		ALTITUDE,
		ALTITUDEENV,
		DENSITY,
		DYNAMICPRESSURE,
		OXYGEN,
		ATMOSPHERE,
		SITUATION,
		REPEATABILITY
	}

	public enum ConstraintTest
	{
		GT,
		LT,
		EQ,
		NEQ
	}

	public enum TestRepeatability
	{
		ALWAYS,
		BODYANDSITUATION,
		ONCEPERPART
	}

	public ConstraintType type;

	public ConstraintTest test = ConstraintTest.EQ;

	public string body = "_Any";

	public string prestige = "_Any";

	public Contract.ContractPrestige prestigeLevel;

	public string value = "";

	public double valueD = double.NaN;

	public bool valueB;

	public uint valueI;

	public TestRepeatability valueR;

	public uint situationMask = uint.MaxValue;

	public bool valid;

	public PartTestConstraint(ConfigNode node = null)
	{
		if (node != null)
		{
			Load(node);
		}
	}

	public bool SituationAvailable(Vessel.Situations sit)
	{
		if (!valid)
		{
			return false;
		}
		return ((uint)sit & situationMask) != 0;
	}

	public bool BodyAvailable(CelestialBody cb)
	{
		if (body == "_Any")
		{
			return true;
		}
		if (body == "_Home" && cb.isHomeWorld)
		{
			return true;
		}
		if (body == "_NotHome" && !cb.isHomeWorld)
		{
			return true;
		}
		if (body == "_Sun" && cb == Planetarium.fetch.Sun)
		{
			return true;
		}
		if (body == "_NotSun" && cb != Planetarium.fetch.Sun)
		{
			return true;
		}
		if (body == cb.bodyName)
		{
			return true;
		}
		return false;
	}

	public bool PrestigeAvailable(Contract.ContractPrestige pres)
	{
		if (prestige == "_Any")
		{
			return true;
		}
		if (pres == prestigeLevel)
		{
			return true;
		}
		return false;
	}

	public bool Test(double input)
	{
		if (!valid)
		{
			return false;
		}
		switch (test)
		{
		default:
			return false;
		case ConstraintTest.GT:
			return input > valueD;
		case ConstraintTest.LT:
			return input < valueD;
		case ConstraintTest.EQ:
			return input == valueD;
		case ConstraintTest.NEQ:
			return input != valueD;
		}
	}

	public bool Test(int input)
	{
		if (!valid)
		{
			return false;
		}
		switch (test)
		{
		default:
			return false;
		case ConstraintTest.GT:
			return input > valueI;
		case ConstraintTest.LT:
			return input < valueI;
		case ConstraintTest.EQ:
			return input == valueI;
		case ConstraintTest.NEQ:
			return input != valueI;
		}
	}

	public bool Test(bool input)
	{
		if (!valid)
		{
			return false;
		}
		if (test == ConstraintTest.EQ)
		{
			return input == valueB;
		}
		return input != valueB;
	}

	public bool Test(string input)
	{
		if (!valid)
		{
			return false;
		}
		if (test == ConstraintTest.EQ)
		{
			return input == value;
		}
		return input != value;
	}

	public bool Test(uint sitmask)
	{
		if (!valid)
		{
			return false;
		}
		if (test == ConstraintTest.EQ)
		{
			return (sitmask & valueI) != 0;
		}
		return (sitmask & valueI) == 0;
	}

	public void Load(ConfigNode node)
	{
		valid = true;
		if (node.HasValue("type"))
		{
			try
			{
				type = (ConstraintType)Enum.Parse(typeof(ConstraintType), node.GetValue("type"));
			}
			catch
			{
				Debug.LogError("[PartTestConstraint]: Error parsing type " + node.GetValue("type") + " in constraint for ModuleTestSubject.");
				valid = false;
			}
		}
		else
		{
			valid = false;
		}
		if (node.HasValue("test"))
		{
			try
			{
				test = (ConstraintTest)Enum.Parse(typeof(ConstraintTest), node.GetValue("test"));
			}
			catch
			{
				Debug.LogError("[PartTestConstraint]: Error parsing type " + node.GetValue("type") + " in constraint for ModuleTestSubject.");
				valid = false;
			}
		}
		if (node.HasValue("value"))
		{
			value = node.GetValue("value");
			if (double.TryParse(value, out var result))
			{
				valueD = result;
			}
			bool flag = bool.TryParse(value, out valueB);
			if (uint.TryParse(value, out var result2))
			{
				valueI = result2;
			}
			if (type != ConstraintType.ALTITUDE && type != ConstraintType.DENSITY && type != ConstraintType.DYNAMICPRESSURE && type != 0)
			{
				if (type != ConstraintType.ATMOSPHERE && type != ConstraintType.OXYGEN)
				{
					if (type == ConstraintType.REPEATABILITY)
					{
						try
						{
							valueR = (TestRepeatability)Enum.Parse(typeof(TestRepeatability), value);
						}
						catch
						{
							Debug.LogError("[PartTestConstraint]: Error parsing value of type repeatability, value =  " + value + " in constraint for ModuleTestSubject.");
							valid = false;
						}
					}
				}
				else if (!flag)
				{
					valid = false;
				}
			}
			else if (double.IsNaN(valueD))
			{
				valid = false;
			}
		}
		if (node.HasValue("body"))
		{
			body = node.GetValue("body");
		}
		if (node.HasValue("situationMask") && uint.TryParse(node.GetValue("situationMask"), out var result3))
		{
			situationMask = result3;
		}
		if (!node.HasValue("prestige"))
		{
			return;
		}
		prestige = node.GetValue("prestige");
		if (prestige != "_Any")
		{
			try
			{
				prestigeLevel = (Contract.ContractPrestige)Enum.Parse(typeof(Contract.ContractPrestige), prestige);
			}
			catch
			{
				Debug.LogError("[PartTestConstraint]: Error parsing prestige level " + prestige + " in constraint for ModuleTestSubject.");
				valid = false;
			}
		}
	}

	public void Save(ConfigNode node)
	{
	}

	public static bool SituationAvailable(List<PartTestConstraint> constraints, Vessel.Situations sit)
	{
		int num = 0;
		int count = constraints.Count;
		while (true)
		{
			if (num < count)
			{
				PartTestConstraint partTestConstraint = constraints[num];
				if (partTestConstraint.type == ConstraintType.SITUATION && !partTestConstraint.Test((uint)sit))
				{
					break;
				}
				num++;
				continue;
			}
			return true;
		}
		return false;
	}
}
