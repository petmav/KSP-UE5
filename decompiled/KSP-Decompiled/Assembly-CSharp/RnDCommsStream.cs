using System;
using System.Collections;
using UnityEngine;

[Serializable]
public class RnDCommsStream
{
	public ScienceSubject subject;

	public float fileSize;

	public float timeout;

	public float xmitValue;

	public float scienceValueRatio;

	public bool xmitIncomplete;

	private float dataIn;

	private float dataOut;

	private float UTofLastTransmit;

	private bool timedOut;

	private ResearchAndDevelopment host;

	public RnDCommsStream(ScienceSubject subject, float fileSize, float timeout, float xmitValue, bool xmitIncomplete, ResearchAndDevelopment RDInstance)
		: this(subject, fileSize, timeout, xmitValue, 1f, xmitIncomplete, RDInstance)
	{
	}

	public RnDCommsStream(ScienceSubject subject, float fileSize, float timeout, float xmitValue, float scienceValueRatio, bool xmitIncomplete, ResearchAndDevelopment RDInstance)
	{
		this.subject = subject;
		this.fileSize = fileSize;
		this.timeout = timeout;
		this.xmitValue = xmitValue;
		this.xmitIncomplete = xmitIncomplete;
		host = RDInstance;
		this.scienceValueRatio = scienceValueRatio;
		dataIn = 0f;
		dataOut = 0f;
		timedOut = true;
	}

	public void StreamData(float dataAmount, ProtoVessel source)
	{
		dataIn = Mathf.Min(fileSize, dataIn + dataAmount);
		UTofLastTransmit = Time.time;
		if (dataIn == fileSize)
		{
			submitStreamData(source);
		}
		else if (timedOut)
		{
			host.StartCoroutine(timeoutCoroutine(source));
		}
	}

	private IEnumerator timeoutCoroutine(ProtoVessel source)
	{
		timedOut = false;
		while (!timedOut && !(dataIn >= fileSize))
		{
			yield return new WaitForSeconds(1f);
			if (Time.time > UTofLastTransmit + timeout)
			{
				timedOut = true;
				if (!xmitIncomplete && dataIn < fileSize)
				{
					dataIn = 0f;
					dataOut = 0f;
					break;
				}
				submitStreamData(source);
			}
		}
	}

	private void submitStreamData(ProtoVessel source)
	{
		float num = dataIn - dataOut;
		if (num > 0f)
		{
			host.SubmitScienceData(num, scienceValueRatio, subject, xmitValue, source);
		}
		dataOut += num;
	}
}
