import unreal
import argparse

# Info about Python with IKRigs here:
# https://docs.unrealengine.com/5.2/en-US/using-python-to-create-and-edit-ik-rigs-in-unreal-engine/
# https://docs.unrealengine.com/5.2/en-US/using-python-to-create-and-edit-ik-retargeter-assets-in-unreal-engine/

parser = argparse.ArgumentParser(description = 'Creates an IKRig given a SkeletalMesh')
parser.add_argument('--skeletalMesh', help='Skeletal Mesh to Use')
args = parser.parse_args()

# Find or Create the IKRig
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
package_path = args.skeletalMesh.rsplit('/', 1)[0] + '/'
asset_name = args.skeletalMesh.split('.')[-1] + '_IKRig'
ikr = skel_mesh = unreal.load_asset(name = package_path + asset_name )
if not ikr:
    ikr = asset_tools.create_asset( asset_name=asset_name,
                                    package_path=package_path, 
                                    asset_class=unreal.IKRigDefinition, 
                                    factory=unreal.IKRigDefinitionFactory())

# Get the controller
ikr_controller = unreal.IKRigController.get_controller(ikr)

# Set the skeletal mesh
skel_mesh = unreal.load_asset(name = args.skeletalMesh)
ikr_controller.set_skeletal_mesh(skel_mesh)

# Get the bone list
bone_names = unreal.DazToUnrealBlueprintUtils.get_bone_list(ikr)

# Use bone list to guess character type
character_type = "unknown"
if all(x in bone_names for x in ['hip', 'pelvis', 'spine1', 'spine4']): character_type = 'Genesis9'
if all(x in bone_names for x in ['hip', 'abdomenLower', 'abdomenUpper', 'chestUpper']): character_type = 'Genesis8'
if all(x in bone_names for x in ['hip', 'abdomenLower', 'abdomenUpper', 'chestUpper', 'lHeel']): character_type = 'Genesis3'
if all(x in bone_names for x in ['Hips', 'HeadTop_End']): character_type = 'Mixamo'

if not character_type == "unknown":

    # Setup goal names
    chains = []
    CHAIN_NAME  = 0
    CHAIN_START = 1
    CHAIN_END   = 2
    CHAIN_GOAL  = 3

    # Define the retarget chains based on the character
    if character_type == 'Genesis9':
        retarget_root_name = 'hip'

        chains.append(['Spine', 'spine1', 'spine4', None])
        chains.append(['Head', 'neck1', 'head', None])

        chains.append(['LeftArm', 'l_upperarm', 'l_hand', 'l_hand_Goal'])
        chains.append(['RightArm', 'r_upperarm', 'r_hand', 'r_hand_Goal'])

        chains.append(['LeftClavicle', 'l_shoulder', 'l_shoulder', None])
        chains.append(['RightClavicle', 'r_shoulder', 'r_shoulder', None])

        chains.append(['LeftLeg', 'l_thigh', 'l_toes', 'l_toes_Goal'])
        chains.append(['RightLeg', 'r_thigh', 'r_toes', 'r_toes_Goal'])

        chains.append(['LeftPinky', 'l_pinky1', 'l_pinky3', None])
        chains.append(['RightPinky', 'r_pinky1', 'r_pinky3', None])
        chains.append(['LeftRing', 'l_ring1', 'l_ring3', None])
        chains.append(['RightRing', 'r_ring1', 'r_ring3', None])
        chains.append(['LeftMiddle', 'l_mid1', 'l_mid3', None])
        chains.append(['RightMiddle', 'r_mid1', 'r_mid3', None])
        chains.append(['LeftIndex', 'l_index1', 'l_index3', None])
        chains.append(['RightIndex', 'r_index1', 'r_index3', None])
        chains.append(['LeftThumb', 'l_thumb1', 'l_thumb3', None])
        chains.append(['RightThumb', 'r_thumb1', 'r_thumb3', None])

        chains.append(['HandRootIK', 'ik_hand_root', 'ik_hand_root', None])
        chains.append(['RightHandIK', 'ik_hand_r', 'ik_hand_r', None])
        chains.append(['HandGunIK', 'ik_hand_gun', 'ik_hand_gun', None])

        chains.append(['FootRootIK', 'ik_foot_root', 'ik_foot_root', None])
        chains.append(['LeftFootIK', 'ik_foot_l', 'ik_foot_l', None])
        chains.append(['RightFootIK', 'ik_foot_r', 'ik_foot_r', None])

        chains.append(['Root', 'root', 'root', None])

    if character_type == 'Genesis8' or character_type == 'Genesis3':
        retarget_root_name = 'hip'

        chains.append(['Spine', 'abdomenLower', 'chestUpper', None])
        chains.append(['Head', 'neckLower', 'head', None])

        chains.append(['LeftArm', 'lShldrBend', 'lHand', 'lHand_Goal'])
        chains.append(['RightArm', 'rShldrBend', 'rHand', 'rHand_Goal'])

        chains.append(['LeftClavicle', 'lCollar', 'lCollar', None])
        chains.append(['RightClavicle', 'rCollar', 'rCollar', None])

        chains.append(['LeftLeg', 'lThighBend', 'lToe', 'lToe_Goal'])
        chains.append(['RightLeg', 'rThighBend', 'rToe', 'rToe_Goal'])

        chains.append(['LeftPinky', 'lPinky1', 'lPinky3', None])
        chains.append(['RightPinky', 'rPinky1', 'rPinky3', None])
        chains.append(['LeftRing', 'lRing1', 'lRing3', None])
        chains.append(['RightRing', 'rRing1', 'rRing3', None])
        chains.append(['LeftMiddle', 'lMid1', 'lMid3', None])
        chains.append(['RightMiddle', 'rMid1', 'rMid3', None])
        chains.append(['LeftIndex', 'lIndex1', 'lIndex3', None])
        chains.append(['RightIndex', 'rIndex1', 'rIndex3', None])
        chains.append(['LeftThumb', 'lThumb1', 'lThumb3', None])
        chains.append(['RightThumb', 'rThumb1', 'rThumb3', None])

        chains.append(['HandRootIK', 'ik_hand_root', 'ik_hand_root', None])
        chains.append(['RightHandIK', 'ik_hand_r', 'ik_hand_r', None])
        chains.append(['HandGunIK', 'ik_hand_gun', 'ik_hand_gun', None])

        chains.append(['FootRootIK', 'ik_foot_root', 'ik_foot_root', None])
        chains.append(['LeftFootIK', 'ik_foot_l', 'ik_foot_l', None])
        chains.append(['RightFootIK', 'ik_foot_r', 'ik_foot_r', None])

        chains.append(['Root', 'root', 'root', None])

    if character_type == 'Mixamo':
        retarget_root_name = 'Hips'

        chains.append(['Spine', 'Spine', 'Spine2', None])
        chains.append(['Head', 'Neck', 'Head', None])

        chains.append(['LeftArm', 'LeftArm', 'LeftHand', 'LeftHand_Goal'])
        chains.append(['RightArm', 'RightArm', 'RightHand', 'RightHand_Goal'])

        chains.append(['LeftClavicle', 'LeftShoulder', 'LeftShoulder', None])
        chains.append(['RightClavicle', 'RightShoulder', 'RightShoulder', None])

        chains.append(['LeftLeg', 'LeftUpLeg', 'LeftToeBase', 'LeftToeBase_Goal'])
        chains.append(['RightLeg', 'RightUpLeg', 'RightToeBase', 'RightToeBase_Goal'])

        chains.append(['LeftPinky', 'LeftHandPinky1', 'LeftHandPinky3', None])
        chains.append(['RightPinky', 'RightHandPinky1', 'RightHandPinky3', None])
        chains.append(['LeftRing', 'LeftHandRing1', 'LeftHandRing3', None])
        chains.append(['RightRing', 'RightHandRing1', 'RightHandRing3', None])
        chains.append(['LeftMiddle', 'LeftHandMiddle1', 'LeftHandMiddle3', None])
        chains.append(['RightMiddle', 'RightHandMiddle1', 'RightHandMiddle3', None])
        chains.append(['LeftIndex', 'LeftHandIndex1', 'LeftHandIndex3', None])
        chains.append(['RightIndex', 'RightHandIndex1', 'RightHandIndex3', None])
        chains.append(['LeftThumb', 'LeftHandThumb1', 'LeftHandThumb3', None])
        chains.append(['RightThumb', 'RightHandThumb1', 'RightHandThumb3', None])


    # Create the solver, remove any existing ones
    while ikr_controller.get_solver_at_index(0):
        ikr_controller.remove_solver(0)
    fbik_index = ikr_controller.add_solver(unreal.IKRigFBIKSolver)

    # Setup root
    ikr_controller.set_root_bone(retarget_root_name, fbik_index)
    ikr_controller.set_retarget_root(retarget_root_name)

    # Remove existing goals
    for goal in ikr_controller.get_all_goals():
        ikr_controller.remove_goal(goal.get_editor_property('goal_name'))

    # Create the goals
    for chain in chains:
        if chain[CHAIN_GOAL]:
            ikr_controller.add_new_goal(chain[CHAIN_GOAL], chain[CHAIN_END])
            ikr_controller.connect_goal_to_solver(chain[CHAIN_GOAL], fbik_index)

    # Remove old Retarget Chains
    for chain in ikr_controller.get_retarget_chains():
        ikr_controller.remove_retarget_chain(chain.get_editor_property('chain_name'))

    # Setup Retarget Chains
    for chain in chains:
        if chain[CHAIN_GOAL]:
            ikr_controller.add_retarget_chain(chain[CHAIN_NAME], chain[CHAIN_START], chain[CHAIN_END], chain[CHAIN_GOAL])
        else:
            ikr_controller.add_retarget_chain(chain[CHAIN_NAME], chain[CHAIN_START], chain[CHAIN_END], '')