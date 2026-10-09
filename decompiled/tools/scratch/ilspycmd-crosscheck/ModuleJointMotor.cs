using System.Collections;
using UnityEngine;

public abstract class ModuleJointMotor : PartModule, IJointLockState
{
	public enum Mode
	{
		NoJoint,
		Park,
		Neutral,
		Drive
	}

	[KSPField]
	public string jointNodeName = "";

	private Vector3 dPos;

	private Quaternion dRot;

	private Vector3 axis;

	private Vector3 secAxis;

	private JointDrive angXDrive;

	private Vector3 initOrt;

	private Vector3 endOrt;

	private Vector3 ctrlAxis;

	private float xAngle;

	private float lastXAngle;

	public Mode mode;

	protected AttachNode refNode { get; private set; }

	protected PartJoint pJoint { get; private set; }

	protected ConfigurableJoint joint { get; private set; }

	public override void OnLoad(ConfigNode node)
	{
		OnModuleLoad(node);
	}

	public override void OnSave(ConfigNode node)
	{
		if (pJoint != null)
		{
			applyCoordsUpdate();
		}
		OnModuleSave(node);
	}

	public override void OnStart(StartState state)
	{
		mode = Mode.NoJoint;
		if (jointNodeName != string.Empty)
		{
			refNode = base.part.FindAttachNode(jointNodeName);
		}
		else
		{
			refNode = base.part.FindAttachNodeByPart(base.part.parent);
		}
		if (refNode != null)
		{
			if (HighLogic.LoadedSceneIsFlight)
			{
				StartCoroutine(startAfterJointsCreated());
			}
		}
		else
		{
			Debug.LogError("[ModuleJointMotor]: Cannot initialize, no attachment node found with id " + jointNodeName, base.gameObject);
		}
		OnModuleStart(state);
	}

	private IEnumerator startAfterJointsCreated()
	{
		while (!base.part.started)
		{
			yield return null;
		}
		InitJoint();
	}

	protected void InitJoint()
	{
		pJoint = findJointAtNode(refNode);
		if (pJoint != null)
		{
			joint = pJoint.Joint;
			axis = joint.axis;
			secAxis = joint.secondaryAxis;
			angXDrive = joint.angularXDrive;
			xAngle = 0f;
			lastXAngle = 0f;
			initOrt = getControlOrt(joint.secondaryAxis);
			dPos = Vector3.zero;
			dRot = Quaternion.identity;
			GameEvents.onPartJointBreak.Add(onPartJointBreak);
			mode = Mode.Park;
			OnJointInit(goodSetup: true);
		}
		else
		{
			OnJointInit(goodSetup: false);
		}
	}

	protected PartJoint findJointAtNode(AttachNode node)
	{
		if (node.attachedPart != null)
		{
			if (node.attachedPart == base.part.parent)
			{
				return base.part.attachJoint;
			}
			return node.attachedPart.attachJoint;
		}
		return null;
	}

	private void onPartJointBreak(PartJoint pj, float force)
	{
		if (mode != 0 && pJoint == pj)
		{
			SetMotorMode(Mode.NoJoint);
			GameEvents.onPartJointBreak.Remove(onPartJointBreak);
		}
	}

	private void applyCoordsUpdate()
	{
		endOrt = getControlOrt(secAxis);
		xAngle = KSPUtil.HeadingDegrees(initOrt, endOrt, getControlOrt(axis));
		if (xAngle - lastXAngle != 0f)
		{
			joint.targetRotation = Quaternion.AngleAxis(xAngle, Vector3.left);
			dRot = Quaternion.AngleAxis(xAngle - lastXAngle, Quaternion.Inverse(pJoint.Host.vessel.transform.rotation) * pJoint.Host.partTransform.rotation * ((pJoint.Child == pJoint.Host) ? axis : (-axis)));
			lastXAngle = xAngle;
			xAngle = 0f;
			recurseCoordUpdate(pJoint.Child, dPos, dRot, Part.PartToVesselSpacePos(joint.anchor, pJoint.Host, pJoint.Host.vessel, PartSpaceMode.Current));
			dRot = Quaternion.identity;
			dPos.Zero();
		}
	}

	private void recurseCoordUpdate(Part p, Vector3 dPos, Quaternion dRot, Vector3 pivot)
	{
		p.orgPos = dRot * (p.orgPos - pivot) + pivot + dRot * dPos;
		p.orgRot = dRot * p.orgRot;
		for (int i = 0; i < p.children.Count; i++)
		{
			recurseCoordUpdate(p.children[i], dPos, dRot, pivot);
		}
	}

	private Vector3 getControlOrt(Vector3 refAxis)
	{
		return Quaternion.Inverse(pJoint.Target.partTransform.rotation) * pJoint.Host.partTransform.rotation * refAxis;
	}

	public void OnPartPack()
	{
		SetMotorMode(Mode.Park);
	}

	protected void SetMotorSpeed(float motorSpeed)
	{
		if (mode == Mode.NoJoint)
		{
			Debug.LogError("[ModuleJointMotor]: Cannot set speed, no joint present.");
		}
		else
		{
			joint.targetAngularVelocity = Vector3.right * motorSpeed;
		}
	}

	protected float GetMotorSpeed()
	{
		if (mode == Mode.NoJoint)
		{
			return 0f;
		}
		return joint.targetAngularVelocity.x;
	}

	protected void SetMotorForce(float motorForce)
	{
		if (mode == Mode.NoJoint)
		{
			Debug.LogError("[ModuleJointMotor]: Cannot set force, no joint present.");
			return;
		}
		angXDrive.maximumForce = motorForce;
		joint.angularXDrive = angXDrive;
	}

	protected float GetMotorForce()
	{
		if (mode == Mode.NoJoint)
		{
			return 0f;
		}
		return angXDrive.maximumForce;
	}

	protected bool SetMotorMode(Mode m)
	{
		if (joint == null)
		{
			return false;
		}
		switch (m)
		{
		case Mode.Park:
			joint.angularXDrive = angXDrive;
			applyCoordsUpdate();
			break;
		case Mode.Neutral:
			joint.angularXDrive = angXDrive;
			break;
		case Mode.Drive:
			joint.angularXDrive = angXDrive;
			break;
		}
		if (m != mode)
		{
			mode = m;
			OnMotorModeChanged(mode);
		}
		base.vessel.CycleAllAutoStrut();
		return true;
	}

	protected Mode GetMotorMode()
	{
		return mode;
	}

	public bool IsJointUnlocked()
	{
		if (joint != null)
		{
			return mode != Mode.Park;
		}
		return false;
	}

	protected abstract void OnModuleSave(ConfigNode node);

	protected abstract void OnModuleLoad(ConfigNode node);

	protected abstract void OnModuleStart(StartState st);

	protected abstract void OnJointInit(bool goodSetup);

	protected abstract void OnMotorModeChanged(Mode mode);
}
