using KSPAchievements;

namespace FinePrint.Utilities
{
	public class ProgressMilestone
	{
		public CelestialBody body;

		public ProgressType type;

		public MannedStatus manned;

		private CelestialBodySubtree progressTree;

		public CelestialBodySubtree progress
		{
			get
			{
				if (progressTree != null)
				{
					return progressTree;
				}
				if (body == null)
				{
					return null;
				}
				progressTree = body.progressTree;
				if (progressTree != null)
				{
					return progressTree;
				}
				int num = ProgressTracking.Instance.celestialBodyNodes.Length;
				while (num-- > 0)
				{
					CelestialBodySubtree celestialBodySubtree = ProgressTracking.Instance.celestialBodyNodes[num];
					if (!(celestialBodySubtree.Body != body))
					{
						progressTree = celestialBodySubtree;
						break;
					}
				}
				return progressTree;
			}
		}

		public bool complete
		{
			get
			{
				if (!possible)
				{
					return false;
				}
				switch (type)
				{
				case ProgressType.SPEEDRECORD:
					return CheckCompletion("RecordsSpeed");
				case ProgressType.REACHSPACE:
					return CheckCompletion("ReachedSpace");
				case ProgressType.ALTITUDERECORD:
					return CheckCompletion("RecordsAltitude");
				case ProgressType.CREWRECOVERY:
					return CheckCompletion("FirstCrewToSurvive");
				case ProgressType.DEPTHRECORD:
					return CheckCompletion("RecordsDepth");
				case ProgressType.DISTANCERECORD:
					return CheckCompletion("RecordsDistance");
				default:
					if (progress != null && progress.IsReached)
					{
						switch (type)
						{
						case ProgressType.BASECONSTRUCTION:
							return CheckCompletion(progress.baseConstruction);
						case ProgressType.CREWTRANSFER:
							return CheckCompletion(progress.crewTransfer);
						case ProgressType.DOCKING:
							return CheckCompletion(progress.docking);
						case ProgressType.ESCAPE:
							return CheckCompletion(progress.escape);
						case ProgressType.FLAGPLANT:
							return CheckCompletion(progress.flagPlant);
						case ProgressType.FLIGHT:
							return CheckCompletion(progress.flight);
						case ProgressType.FLYBY:
							return CheckCompletion(progress.flyBy);
						case ProgressType.FLYBYRETURN:
							return CheckCompletion(progress.returnFromFlyby);
						case ProgressType.LANDING:
							return CheckCompletion(progress.landing);
						case ProgressType.LANDINGRETURN:
							return CheckCompletion(progress.returnFromSurface);
						case ProgressType.ORBIT:
							return CheckCompletion(progress.orbit);
						case ProgressType.ORBITRETURN:
							return CheckCompletion(progress.returnFromOrbit);
						case ProgressType.RENDEZVOUS:
							return CheckCompletion(progress.rendezvous);
						case ProgressType.SCIENCE:
							return CheckCompletion(progress.science);
						case ProgressType.SPACEWALK:
							return CheckCompletion(progress.spacewalk);
						case ProgressType.SPLASHDOWN:
							return CheckCompletion(progress.splashdown);
						case ProgressType.STATIONCONSTRUCTION:
							return CheckCompletion(progress.stationConstruction);
						default:
							return false;
						case ProgressType.SUBORBIT:
							return CheckCompletion(progress.suborbit);
						case ProgressType.SURFACEEVA:
							return CheckCompletion(progress.surfaceEVA);
						}
					}
					return false;
				case ProgressType.FIRSTLAUNCH:
					return CheckCompletion("FirstLaunch");
				}
			}
		}

		public bool possible
		{
			get
			{
				if (type == ProgressType.SPLASHDOWN && (body == null || !body.ocean))
				{
					return false;
				}
				if (type == ProgressType.FLIGHT && (body == null || !body.atmosphere))
				{
					return false;
				}
				if (manned == MannedStatus.UNMANNED && impliesManned)
				{
					return false;
				}
				if (CelestialUtilities.IsGasGiant(body))
				{
					switch (type)
					{
					case ProgressType.BASECONSTRUCTION:
					case ProgressType.FLAGPLANT:
					case ProgressType.LANDING:
					case ProgressType.LANDINGRETURN:
					case ProgressType.SURFACEEVA:
						return false;
					}
				}
				return true;
			}
		}

		public bool bodySensitive
		{
			get
			{
				switch (type)
				{
				default:
					return false;
				case ProgressType.BASECONSTRUCTION:
				case ProgressType.CREWTRANSFER:
				case ProgressType.DOCKING:
				case ProgressType.ESCAPE:
				case ProgressType.FLAGPLANT:
				case ProgressType.FLIGHT:
				case ProgressType.FLYBY:
				case ProgressType.FLYBYRETURN:
				case ProgressType.LANDING:
				case ProgressType.LANDINGRETURN:
				case ProgressType.ORBIT:
				case ProgressType.ORBITRETURN:
				case ProgressType.RENDEZVOUS:
				case ProgressType.SCIENCE:
				case ProgressType.SPACEWALK:
				case ProgressType.SPLASHDOWN:
				case ProgressType.STATIONCONSTRUCTION:
				case ProgressType.SUBORBIT:
				case ProgressType.SURFACEEVA:
					return true;
				}
			}
		}

		public bool crewSensitive
		{
			get
			{
				switch (type)
				{
				default:
					return false;
				case ProgressType.BASECONSTRUCTION:
				case ProgressType.DOCKING:
				case ProgressType.ESCAPE:
				case ProgressType.FIRSTLAUNCH:
				case ProgressType.FLIGHT:
				case ProgressType.FLYBY:
				case ProgressType.FLYBYRETURN:
				case ProgressType.LANDING:
				case ProgressType.LANDINGRETURN:
				case ProgressType.ORBIT:
				case ProgressType.ORBITRETURN:
				case ProgressType.REACHSPACE:
				case ProgressType.RENDEZVOUS:
				case ProgressType.SCIENCE:
				case ProgressType.SPLASHDOWN:
				case ProgressType.STATIONCONSTRUCTION:
				case ProgressType.SUBORBIT:
					return true;
				}
			}
		}

		public bool impliesManned
		{
			get
			{
				switch (type)
				{
				default:
					return false;
				case ProgressType.CREWTRANSFER:
				case ProgressType.CREWRECOVERY:
				case ProgressType.FLAGPLANT:
				case ProgressType.SPACEWALK:
				case ProgressType.SURFACEEVA:
					return true;
				}
			}
		}

		public ProgressMilestone()
		{
			body = Planetarium.fetch.Sun;
			type = ProgressType.SPLASHDOWN;
			manned = MannedStatus.ANY;
		}

		public ProgressMilestone(CelestialBody body, ProgressType type, MannedStatus manned = MannedStatus.ANY)
		{
			this.body = body;
			this.type = type;
			this.manned = manned;
		}

		private bool CheckCompletion(ProgressNode node)
		{
			if (node == null)
			{
				return false;
			}
			switch (manned)
			{
			default:
				return node.IsComplete;
			case MannedStatus.UNMANNED:
				return node.IsCompleteUnmanned;
			case MannedStatus.MANNED:
				return node.IsCompleteManned;
			}
		}

		private bool CheckCompletion(string nodeID)
		{
			if (ProgressTracking.Instance == null)
			{
				return false;
			}
			switch (manned)
			{
			default:
				return ProgressTracking.Instance.NodeComplete(nodeID);
			case MannedStatus.UNMANNED:
				return ProgressTracking.Instance.NodeCompleteUnmanned(nodeID);
			case MannedStatus.MANNED:
				return ProgressTracking.Instance.NodeCompleteManned(nodeID);
			}
		}
	}
}
