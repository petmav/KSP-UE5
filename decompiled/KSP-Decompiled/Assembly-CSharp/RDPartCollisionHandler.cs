using System;
using UnityEngine;

public class RDPartCollisionHandler : MonoBehaviour
{
	public KerbalEVA eva;

	private void Start()
	{
		if (!eva)
		{
			throw new Exception("[Ragdoll Error]: Ragdoll Part Collision Handler's EVA reference is null");
		}
	}

	private void OnCollisionEnter(Collision c)
	{
		if ((bool)eva)
		{
			eva.OnCollisionEnter(c);
		}
	}

	private void OnCollisionStay(Collision c)
	{
		if ((bool)eva)
		{
			eva.OnCollisionStay(c);
		}
	}

	private void OnCollisionExit(Collision c)
	{
		if ((bool)eva)
		{
			eva.OnCollisionExit(c);
		}
	}
}
