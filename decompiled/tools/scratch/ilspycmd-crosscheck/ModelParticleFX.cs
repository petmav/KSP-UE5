using UnityEngine;

[EffectDefinition("MODEL_PARTICLE")]
public class ModelParticleFX : EffectBehaviour
{
	[Persistent]
	public string modelName = "";

	[Persistent]
	public string transformName = "";

	[Persistent]
	public Vector3 localRotation = Vector3.zero;

	[Persistent]
	public Vector3 localPosition = Vector3.zero;

	[Persistent]
	public Vector3 localScale = Vector3.one;

	public FXCurve emission = new FXCurve("emission", 1f);

	public FXCurve energy = new FXCurve("energy", 1f);

	public FXCurve speed = new FXCurve("speed", 1f);

	private Transform modelParent;

	private KSPParticleEmitter emitter;

	private float minEmission;

	private float maxEmission;

	private float minEnergy;

	private float maxEnergy;

	private Vector3 localVelocity;

	private float emissionPower;

	private float energyPower;

	private float speedPower;

	public override void OnLoad(ConfigNode node)
	{
		ConfigNode.LoadObjectFromConfig(this, node);
		emission.Load("emission", node);
		energy.Load("energy", node);
		speed.Load("speed", node);
	}

	public override void OnSave(ConfigNode node)
	{
		ConfigNode.CreateConfigFromObject(this, node);
		emission.Save(node);
		energy.Save(node);
		speed.Save(node);
	}

	public override void OnInitialize()
	{
		modelParent = hostPart.FindModelTransform(transformName);
		if (modelParent == null)
		{
			Debug.LogError("ParticleModelFX: Cannot find transform of name '" + transformName + "'");
			return;
		}
		GameObject model = GameDatabase.Instance.GetModel(modelName);
		if (model == null)
		{
			Debug.LogError("ParticleModelFX: Cannot find model of name '" + modelName + "'");
			return;
		}
		model.SetActive(value: true);
		emitter = model.GetComponentInChildren<KSPParticleEmitter>();
		if (emitter == null)
		{
			Debug.LogError("ParticleModelFX: Cannot find particle emitter on model of name '" + modelName + "'");
			Object.Destroy(model);
			return;
		}
		emitter.transform.NestToParent(modelParent);
		emitter.transform.localPosition = localPosition;
		emitter.transform.localRotation = Quaternion.Euler(localRotation);
		emitter.transform.localScale = Vector3.Scale(emitter.transform.localScale, localScale);
		minEmission = emitter.minEmission;
		maxEmission = emitter.maxEmission;
		minEnergy = emitter.minEnergy;
		maxEnergy = emitter.maxEnergy;
		localVelocity = emitter.localVelocity;
		EffectBehaviour.AddParticleEmitter(emitter);
	}

	public override void OnEvent()
	{
		if (!(emitter == null))
		{
			emitter.Emit();
		}
	}

	public override void OnEvent(float power)
	{
		if (!(emitter == null))
		{
			if (power <= 0f)
			{
				emitter.emit = false;
				return;
			}
			emitter.emit = true;
			emissionPower = emission.Value(power);
			emitter.minEmission = Mathf.FloorToInt(minEmission * emissionPower);
			emitter.maxEmission = Mathf.FloorToInt(maxEmission * emissionPower);
			energyPower = energy.Value(power);
			emitter.minEnergy = minEnergy * energyPower;
			emitter.maxEnergy = maxEnergy * energyPower;
			speedPower = speed.Value(power);
			emitter.localVelocity = localVelocity * speedPower;
		}
	}

	private void OnDestroy()
	{
		EffectBehaviour.RemoveParticleEmitter(emitter);
	}
}
