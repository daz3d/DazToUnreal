import unreal
import argparse
import json

def get_scriptstruct_by_node_name(node_name):
    control_rig_blueprint = unreal.load_object(None, '/DazToUnreal/Python/ControlRig_Node_Library')
    rig_vm_graph = control_rig_blueprint.get_model()
    nodes = rig_vm_graph.get_nodes()
    for node in nodes:
        if node.get_node_path() == node_name:
            return node.get_script_struct()


last_construction_link = 'PrepareForExecution'
def create_construction(bone_name):
    global last_construction_link
    control_name = bone_name + '_ctrl'
    get_bone_transform_node_name = "RigUnit_Construction_GetTransform_" + bone_name
    set_control_transform_node_name = "RigtUnit_Construction_SetTransform_" + control_name

    rig_controller.add_unit_node_from_struct_path('/Script/ControlRig.RigUnit_GetTransform', 'Execute', unreal.Vector2D(137.732236, -595.972187), get_bone_transform_node_name)
    rig_controller.set_pin_default_value(get_bone_transform_node_name + '.Item', '(Type=Bone)')
    rig_controller.set_pin_expansion(get_bone_transform_node_name + '.Item', True)
    rig_controller.set_pin_default_value(get_bone_transform_node_name + '.Space', 'GlobalSpace')
    rig_controller.set_pin_default_value(get_bone_transform_node_name + '.bInitial', 'True')
    rig_controller.set_pin_default_value(get_bone_transform_node_name + '.Item.Name', bone_name, True)
    rig_controller.set_pin_default_value(get_bone_transform_node_name + '.Item.Type', 'Bone', True)
    rig_controller.set_node_selection([get_bone_transform_node_name])
    try:
        rig_controller.add_template_node('Set Transform::Execute(in Item,in Space,in bInitial,in Value,in Weight,in bPropagateToChildren,io ExecuteContext)', unreal.Vector2D(526.732236, -608.972187), set_control_transform_node_name)
    except Exception as e:
        set_transform_scriptstruct = get_scriptstruct_by_node_name("SetTransform")
        rig_controller.add_unit_node(set_transform_scriptstruct, 'Execute', unreal.Vector2D(526.732236, -608.972187), set_control_transform_node_name)
    rig_controller.set_pin_default_value(set_control_transform_node_name + '.Item', '(Type=Bone,Name="None")')
    rig_controller.set_pin_expansion(set_control_transform_node_name + '.Item', False)
    rig_controller.set_pin_default_value(set_control_transform_node_name + '.Space', 'GlobalSpace')
    rig_controller.set_pin_default_value(set_control_transform_node_name + '.bInitial', 'True')
    rig_controller.set_pin_default_value(set_control_transform_node_name + '.Weight', '1.000000')
    rig_controller.set_pin_default_value(set_control_transform_node_name + '.bPropagateToChildren', 'True')
    rig_controller.set_pin_default_value(set_control_transform_node_name + '.Item.Name', control_name, True)
    rig_controller.set_pin_default_value(set_control_transform_node_name + '.Item.Type', 'Control', True)
    try:
        rig_controller.add_link(get_bone_transform_node_name + '.Transform', set_control_transform_node_name + '.Value')
    except:
        try:
            rig_controller.add_link(get_bone_transform_node_name + '.Transform', set_control_transform_node_name + '.Transform')
        except Exception as e:
            print("ERROR: CreateControlRig.py, line 45: rig_controller.add_link(): " + str(e))
    rig_controller.set_node_position_by_name(set_control_transform_node_name, unreal.Vector2D(512.000000, -656.000000))
    rig_controller.add_link(last_construction_link + '.ExecuteContext', set_control_transform_node_name + '.ExecuteContext')
    last_construction_link = set_control_transform_node_name


last_backward_solver_link = 'InverseExecution.ExecuteContext'
def create_backward_solver(bone_name):
    global last_backward_solver_link
    control_name = bone_name + '_ctrl'
    get_bone_transform_node_name = "RigUnit_BackwardSolver_GetTransform_" + bone_name
    set_control_transform_node_name = "RigtUnit_BackwardSolver_SetTransform_" + control_name

    #rig_controller.add_link('InverseExecution.ExecuteContext', 'RigUnit_SetTransform_3.ExecuteContext')
    rig_controller.add_unit_node_from_struct_path('/Script/ControlRig.RigUnit_GetTransform', 'Execute', unreal.Vector2D(-636.574629, -1370.167943), get_bone_transform_node_name)
    rig_controller.set_pin_default_value(get_bone_transform_node_name + '.Item', '(Type=Bone)')
    rig_controller.set_pin_expansion(get_bone_transform_node_name + '.Item', True)
    rig_controller.set_pin_default_value(get_bone_transform_node_name + '.Space', 'GlobalSpace')
    rig_controller.set_pin_default_value(get_bone_transform_node_name + '.Item.Name', bone_name, True)
    rig_controller.set_pin_default_value(get_bone_transform_node_name + '.Item.Type', 'Bone', True)
 
    try:
        rig_controller.add_template_node('Set Transform::Execute(in Item,in Space,in bInitial,in Value,in Weight,in bPropagateToChildren,io ExecuteContext)', unreal.Vector2D(-190.574629, -1378.167943), set_control_transform_node_name)
    except:
        set_transform_scriptstruct = get_scriptstruct_by_node_name("SetTransform")
        rig_controller.add_unit_node(set_transform_scriptstruct, 'Execute', unreal.Vector2D(-190.574629, -1378.167943), set_control_transform_node_name)
    rig_controller.set_pin_default_value(set_control_transform_node_name + '.Item', '(Type=Bone,Name="None")')
    rig_controller.set_pin_expansion(set_control_transform_node_name + '.Item', False)
    rig_controller.set_pin_default_value(set_control_transform_node_name + '.Space', 'GlobalSpace')
    rig_controller.set_pin_default_value(set_control_transform_node_name + '.bInitial', 'False')
    rig_controller.set_pin_default_value(set_control_transform_node_name + '.Weight', '1.000000')
    rig_controller.set_pin_default_value(set_control_transform_node_name + '.bPropagateToChildren', 'True')
    rig_controller.set_pin_default_value(set_control_transform_node_name + '.Item.Name', control_name, True)
    rig_controller.set_pin_default_value(set_control_transform_node_name + '.Item.Type', 'Control', True)
    #rig_controller.set_pin_default_value(set_control_transform_node_name + '.Transform', '(Rotation=(X=0.000000,Y=0.000000,Z=0.000000,W=-1.000000),Translation=(X=0.551784,Y=-0.000000,Z=72.358307),Scale3D=(X=1.000000,Y=1.000000,Z=1.000000))', True)

    try:
        rig_controller.add_link(get_bone_transform_node_name + '.Transform', set_control_transform_node_name + '.Value')
    except:
        try:
            rig_controller.add_link(get_bone_transform_node_name + '.Transform', set_control_transform_node_name + '.Transform')
        except Exception as e:
            print("ERROR: CreateControlRig.py, line 84: rig_controller.add_link(): " + str(e))
    rig_controller.add_link(last_backward_solver_link, set_control_transform_node_name + '.ExecuteContext')
    last_backward_solver_link = set_control_transform_node_name + '.ExecuteContext'

def create_control(bone_name):
    control_name = bone_name + '_ctrl'
    default_setting = unreal.RigControlSettings()
    default_setting.shape_name = 'Box_Thin'
    default_setting.control_type = unreal.RigControlType.EULER_TRANSFORM
    default_value = hierarchy.make_control_value_from_euler_transform(
    unreal.EulerTransform(scale=[1, 1, 1]))

    key = unreal.RigElementKey(type=unreal.RigElementType.BONE, name=bone_name)
    rig_control_element = hierarchy.find_control(unreal.RigElementKey(type=unreal.RigElementType.CONTROL, name=control_name))
    control_key = rig_control_element.get_editor_property('key')
    print(rig_control_element)
    if control_key.get_editor_property('name') != "":
        control_key = rig_control_element.get_editor_property('key')
    else:
        try:
            control_key = hierarchy_controller.add_control(control_name, unreal.RigElementKey(), default_setting, default_value, True, True)
        except:
            control_key = hierarchy_controller.add_control(control_name, unreal.RigElementKey(), default_setting, default_value, True)
    transform = hierarchy.get_global_transform(key, True)
    hierarchy.set_control_offset_transform(control_key, transform, True)

    if bone_name in control_list:
        create_direct_control(bone_name)
    elif bone_name in effector_list:
        create_effector(bone_name)
    create_construction(bone_name)
    create_backward_solver(bone_name)

effector_get_transform_widget_height = 0
def create_direct_control(bone_name):
    global next_forward_execute
    global effector_get_transform_widget_height
    #effector_get_transform_widget_height += 250
    control_name = bone_name + '_ctrl'
    get_control_transform_node_name = "RigUnit_DirectControl_GetTransform_" + bone_name
    set_bone_transform_node_name = "RigtUnit_DirectControl_SetTransform_" + control_name
    rig_controller.add_unit_node_from_struct_path('/Script/ControlRig.RigUnit_GetTransform', 'Execute', unreal.Vector2D(101.499447, -244.500249), get_control_transform_node_name)
    rig_controller.set_pin_default_value(get_control_transform_node_name + '.Item', '(Type=Bone)')
    rig_controller.set_pin_expansion(get_control_transform_node_name + '.Item', True)
    rig_controller.set_pin_default_value(get_control_transform_node_name + '.Space', 'GlobalSpace')
    rig_controller.set_pin_default_value(get_control_transform_node_name + '.Item.Name', control_name, True)
    rig_controller.set_pin_default_value(get_control_transform_node_name + '.Item.Type', 'Control', True)

    try:
        rig_controller.add_template_node('Set Transform::Execute(in Item,in Space,in bInitial,in Value,in Weight,in bPropagateToChildren,io ExecuteContext)', unreal.Vector2D(542.832780, -257.833582), set_bone_transform_node_name)
    except:
        set_transform_scriptstruct = get_scriptstruct_by_node_name("SetTransform")
        rig_controller.add_unit_node(set_transform_scriptstruct, 'Execute', unreal.Vector2D(542.832780, -257.833582), set_bone_transform_node_name)
    rig_controller.set_pin_default_value(set_bone_transform_node_name + '.Item', '(Type=Bone,Name="None")')
    rig_controller.set_pin_expansion(set_bone_transform_node_name + '.Item', False)
    rig_controller.set_pin_default_value(set_bone_transform_node_name + '.Space', 'GlobalSpace')
    rig_controller.set_pin_default_value(set_bone_transform_node_name + '.bInitial', 'False')
    rig_controller.set_pin_default_value(set_bone_transform_node_name + '.Weight', '1.000000')
    rig_controller.set_pin_default_value(set_bone_transform_node_name + '.bPropagateToChildren', 'True')
    rig_controller.set_pin_default_value(set_bone_transform_node_name + '.Item.Name', bone_name, True)
    rig_controller.set_pin_default_value(set_bone_transform_node_name + '.Item.Type', 'Bone', True)

    try:
        rig_controller.add_link(get_control_transform_node_name + '.Transform', set_bone_transform_node_name + '.Value')
    except:
        try:
            rig_controller.add_link(get_control_transform_node_name + '.Transform', set_bone_transform_node_name + '.Transform')
        except Exception as e:
            print("ERROR: CreateControlRig.py: rig_controller.add_Link(): " + str(e))
    rig_controller.add_link(next_forward_execute, set_bone_transform_node_name + '.ExecuteContext')
    next_forward_execute = set_bone_transform_node_name + '.ExecuteContext'

# def create_preferred_angle_control(bone_name):
#     global next_forward_execute
#     global effector_get_transform_widget_height
#     #effector_get_transform_widget_height += 250
#     control_name = bone_name + '_ctrl'
#     get_control_transform_node_name = "RigUnit_PreferredAngle_GetTransform_" + bone_name
#     set_bone_transform_node_name = "RigtUnit_PreferredAngle_SetTransform_" + control_name
#     rig_controller.add_unit_node_from_struct_path('/Script/ControlRig.RigUnit_GetTransform', 'Execute', unreal.Vector2D(101.499447, -244.500249), get_control_transform_node_name)
#     rig_controller.set_pin_default_value(get_control_transform_node_name + '.Item', '(Type=Bone)')
#     rig_controller.set_pin_expansion(get_control_transform_node_name + '.Item', True)
#     rig_controller.set_pin_default_value(get_control_transform_node_name + '.Space', 'GlobalSpace')
#     rig_controller.set_pin_default_value(get_control_transform_node_name + '.Item.Name', control_name, True)
#     rig_controller.set_pin_default_value(get_control_transform_node_name + '.Item.Type', 'Control', True)

#     try:
#         rig_controller.add_template_node('Set Transform::Execute(in Item,in Space,in bInitial,in Value,in Weight,in bPropagateToChildren,io ExecuteContext)', unreal.Vector2D(542.832780, -257.833582), set_bone_transform_node_name)
#     except:
#         set_transform_scriptstruct = get_scriptstruct_by_node_name("SetTransform")
#         rig_controller.add_unit_node(set_transform_scriptstruct, 'Execute', unreal.Vector2D(542.832780, -257.833582), set_bone_transform_node_name)
#     rig_controller.set_pin_default_value(set_bone_transform_node_name + '.Item', '(Type=Bone,Name="None")')
#     rig_controller.set_pin_expansion(set_bone_transform_node_name + '.Item', False)
#     rig_controller.set_pin_default_value(set_bone_transform_node_name + '.Space', 'GlobalSpace')
#     rig_controller.set_pin_default_value(set_bone_transform_node_name + '.bInitial', 'False')
#     rig_controller.set_pin_default_value(set_bone_transform_node_name + '.Weight', '1.000000')
#     rig_controller.set_pin_default_value(set_bone_transform_node_name + '.bPropagateToChildren', 'True')
#     rig_controller.set_pin_default_value(set_bone_transform_node_name + '.Item.Name', bone_name, True)
#     rig_controller.set_pin_default_value(set_bone_transform_node_name + '.Item.Type', 'Bone', True)

#     try:
#         rig_controller.add_link(get_control_transform_node_name + '.Transform', set_bone_transform_node_name + '.Value')
#     except:
#         try:
#             rig_controller.add_link(get_control_transform_node_name + '.Transform', set_bone_transform_node_name + '.Transform')
#         except Exception as e:
#             print("ERROR: CreateControlRig.py: rig_controller.add_Link(): " + str(e))
#     rig_controller.add_link(next_forward_execute, set_bone_transform_node_name + '.ExecuteContext')
#     next_forward_execute = set_bone_transform_node_name + '.ExecuteContext'
   
def create_effector(bone_name):
    global effector_get_transform_widget_height
    effector_get_transform_widget_height += 250
    control_name = bone_name + '_ctrl'
    get_transform_name = control_name + '_RigUnit_GetTransform'
    pin_name = rig_controller.insert_array_pin('PBIK.Effectors', -1, '')
    print(pin_name)
    rig_controller.add_unit_node_from_struct_path('/Script/ControlRig.RigUnit_GetTransform', 'Execute', unreal.Vector2D(-122.733358, effector_get_transform_widget_height), get_transform_name)
    rig_controller.set_pin_default_value(get_transform_name + '.Item', '(Type=Bone)')
    rig_controller.set_pin_expansion(get_transform_name + '.Item', True)
    rig_controller.set_pin_default_value(get_transform_name + '.Space', 'GlobalSpace')
    rig_controller.set_pin_default_value(get_transform_name + '.Item.Name', control_name, True)
    rig_controller.set_pin_default_value(get_transform_name + '.Item.Type', 'Control', True)
    rig_controller.set_node_selection([get_transform_name])
    rig_controller.add_link(get_transform_name + '.Transform', pin_name + '.Transform')
    rig_controller.set_pin_default_value(pin_name + '.Bone', bone_name, False)
    if(bone_name in guide_list):
        rig_controller.set_pin_default_value(pin_name + '.StrengthAlpha', '0.200000', False)


parser = argparse.ArgumentParser(description = 'Creates a Control Rig given a SkeletalMesh')
parser.add_argument('--skeletalMesh', help='Skeletal Mesh to Use')
parser.add_argument('--dtuFile', help='DTU File to use')
args = parser.parse_args()

asset_name = args.skeletalMesh.split('.')[1] + '_CR'
package_path = args.skeletalMesh.rsplit('/', 1)[0]

blueprint = unreal.load_object(name = package_path + '/' + asset_name , outer = None)
if not blueprint:
    blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(asset_name=asset_name,
                                           package_path=package_path,
                                           asset_class=unreal.ControlRigBlueprint,
                                           factory=unreal.ControlRigBlueprintFactory())

#blueprint = unreal.load_object(name = '/Game/NewControlRigBlueprint.NewControlRigBlueprint', outer = None)

if blueprint: 

    # Turn off notifications or each change will compile the RigVM
    blueprint.suspend_notifications(True)

    library = blueprint.get_local_function_library()
    library_controller = blueprint.get_controller(library)
    hierarchy = blueprint.hierarchy
    hierarchy_controller = hierarchy.get_controller()

    rig_controller = blueprint.get_controller_by_name('RigVMModel')
    if rig_controller is None:
        rig_controller = blueprint.get_controller()

    #rig_controller.set_node_selection(['RigUnit_BeginExecution'])
    hierarchy_controller.import_bones_from_asset(args.skeletalMesh, 'None', True, False, True)

    # Remove Existing Nodes
    graph = rig_controller.get_graph()
    node_count = len(graph.get_nodes())
    while node_count > 0:
        rig_controller.remove_node(graph.get_nodes()[-1])
        node_count = node_count - 1

    # Create Full Body IK Node
    rig_controller.add_unit_node_from_struct_path('/Script/ControlRig.RigUnit_BeginExecution', 'Execute', unreal.Vector2D(22.229613, 60.424645), 'BeginExecution')
    rig_controller.add_unit_node_from_struct_path('/Script/ControlRig.RigUnit_PrepareForExecution', 'Execute', unreal.Vector2D(-216.659278, -515.130927), 'PrepareForExecution')
    rig_controller.add_unit_node_from_struct_path('/Script/ControlRig.RigUnit_InverseExecution', 'Execute', unreal.Vector2D(-307.389434, -270.395477), 'InverseExecution')
    rig_controller.add_unit_node_from_struct_path('/Script/PBIK.RigUnit_PBIK', 'Execute', unreal.Vector2D(370.599976, 42.845032), 'PBIK')
    rig_controller.set_pin_default_value('PBIK.Settings', '(Iterations=20,MassMultiplier=1.000000,MinMassMultiplier=0.200000,bStartSolveFromInputPose=True)')
    rig_controller.set_pin_expansion('PBIK.Settings', False)
    rig_controller.set_pin_default_value('PBIK.Debug', '(DrawScale=1.000000)')
    rig_controller.set_pin_expansion('PBIK.Debug', False)
    rig_controller.set_node_selection(['PBIK'])
    rig_controller.set_pin_default_value('PBIK.Root', 'hip', False)
    rig_controller.insert_array_pin('PBIK.BoneSettings', -1, '')
    rig_controller.set_pin_default_value('PBIK.BoneSettings.0.Bone', 'pelvis', False)
    rig_controller.set_pin_default_value('PBIK.BoneSettings.0.RotationStiffness', '0.900000', False)
    rig_controller.set_pin_default_value('PBIK.BoneSettings.0.PositionStiffness', '0.900000', False)

    next_forward_execute = 'BeginExecution.ExecuteContext'
    #rig_controller.add_link('BeginExecution.ExecuteContext', 'PBIK.ExecuteContext')

    # Get Bone list for character
    dtu_data = json.load(open(args.dtuFile.replace('\"', '')))
    limits = dtu_data['LimitData']
    bones_in_dtu = ['root']
    for bone_limits in limits.values():
        bone_limit_name = bone_limits[0]
        #print(bone_limit_name)
        bones_in_dtu.append(bone_limit_name)

    # Create Effectors, order matters.  Child controls should be after Parent Control
    effector_list = ['pelvis', 'l_foot', 'r_foot', 'spine4', 'l_hand', 'r_hand'] # G9
    effector_list+= ['chestUpper', 'head', 'lFoot', 'rFoot', 'lHand', 'rHand'] #G8
    effector_list = [bone for bone in effector_list if bone in bones_in_dtu]
    #print(effector_list)

    # These controls are mid chain and don't have full weight
    guide_list = ['l_shin', 'r_shin', 'l_forearm', 'r_forearm'] # G9
    guide_list+= ['lShin', 'rShin', 'lForearmBend', 'rForearmBend'] # G8
    guide_list = [bone for bone in guide_list if bone in bones_in_dtu]

    # These controls are for bones that shouldn't move much on their own, but user can rotate them
    suggested_rotation_list = ['l_shoulder', 'r_shoulder'] # G9
    suggested_rotation_list+= ['lCollar', 'rCollar'] # G8
    suggested_rotation_list = [bone for bone in suggested_rotation_list if bone in bones_in_dtu]

    # This is for controls outside of Full Body IK
    control_list = ['root', 'hip']
    control_list = [bone for bone in control_list if bone in bones_in_dtu]

    for effector_name in control_list + effector_list + guide_list:
        create_control(effector_name)

    parent_list = {
        'root':['hip', 'l_foot', 'r_foot', 'l_shin', 'r_shin', 'lFoot', 'rFoot', 'lShin', 'rShin'],
        'hip':['pelvis'],
        'pelvis':['spine4', 'chestUpper'],
        'spine4':['head', 'l_hand', 'r_hand', 'l_forearm', 'r_forearm'],
        'chestUpper':['head', 'lHand', 'rHand', 'lForearmBend', 'rForearmBend']
    }

    for parent_bone, child_bone_list in parent_list.items():
        if not parent_bone in bones_in_dtu: continue
        filtered_child_bone_list = [bone for bone in child_bone_list if bone in bones_in_dtu]
        for child_bone in child_bone_list:
            hierarchy_controller.set_parent(unreal.RigElementKey(type=unreal.RigElementType.CONTROL, name= child_bone + '_ctrl'), unreal.RigElementKey(type=unreal.RigElementType.CONTROL, name= parent_bone + '_ctrl'), True)
            hierarchy.set_local_transform(unreal.RigElementKey(type=unreal.RigElementType.CONTROL, name=child_bone + '_ctrl'), unreal.Transform(location=[0.000000,0.000000,0.000000],rotation=[0.000000,0.000000,-0.000000],scale=[1.000000,1.000000,1.000000]), True, True)
        
    skeletal_mesh = unreal.load_object(name = args.skeletalMesh, outer = None)
    skeletal_mesh_import_data = skeletal_mesh.get_editor_property('asset_import_data')
    skeletal_mesh_force_front_x = skeletal_mesh_import_data.get_editor_property('force_front_x_axis')

    stiff_limits = ['l_shoulder', 'r_shoulder']
    stiff_limits+= ['lCollar', 'rCollar']
    exclude_limits = ['root', 'hip', 'pelvis']
    for bone_limits in limits.values():

        # Get the name of the bone
        bone_limit_name = bone_limits[0]

        # Add twist bones to the exlude list
        if 'twist' in bone_limit_name.lower():
            exluded_bone_settings = rig_controller.insert_array_pin('PBIK.ExcludedBones', -1, '')
            rig_controller.set_pin_default_value(exluded_bone_settings, bone_limit_name, False)

        # Get the bone limits
        bone_limit_x_min = bone_limits[2]
        bone_limit_x_max = bone_limits[3]
        bone_limit_y_min = bone_limits[4] * -1.0
        bone_limit_y_max = bone_limits[5] * -1.0
        bone_limit_z_min = bone_limits[6] * -1.0
        bone_limit_z_max = bone_limits[7] * -1.0

        # update the axis if force front was used (facing right)
        if skeletal_mesh_force_front_x:
            bone_limit_y_min = bone_limits[2] * -1.0
            bone_limit_y_max = bone_limits[3] * -1.0
            bone_limit_z_min = bone_limits[4] * -1.0
            bone_limit_z_max = bone_limits[5] * -1.0
            bone_limit_x_min = bone_limits[6]
            bone_limit_x_max = bone_limits[7]

        if not bone_limit_name in exclude_limits:
            #if not bone_limit_name == "l_shin": continue
            limit_bone_settings = rig_controller.insert_array_pin('PBIK.BoneSettings', -1, '')
            rig_controller.set_pin_default_value(limit_bone_settings + '.Bone', bone_limit_name, False)

            
            x_delta = abs(bone_limit_x_max - bone_limit_x_min)
            y_delta = abs(bone_limit_y_max - bone_limit_y_min)
            z_delta = abs(bone_limit_z_max - bone_limit_z_min)

            if(x_delta < 15.0):
                rig_controller.set_pin_default_value(limit_bone_settings + '.X', 'Locked', False)
            elif (x_delta > 90.0):
                rig_controller.set_pin_default_value(limit_bone_settings + '.X', 'Free', False)
            else:
                rig_controller.set_pin_default_value(limit_bone_settings + '.X', 'Free', False)

            if(y_delta < 15.0):
                rig_controller.set_pin_default_value(limit_bone_settings + '.Y', 'Locked', False)
            elif (y_delta > 90.0):
                rig_controller.set_pin_default_value(limit_bone_settings + '.Y', 'Free', False)
            else:
                rig_controller.set_pin_default_value(limit_bone_settings + '.Y', 'Free', False)

            if(z_delta < 15.0):
                rig_controller.set_pin_default_value(limit_bone_settings + '.Z', 'Locked', False)
            elif (z_delta > 90.0):
                rig_controller.set_pin_default_value(limit_bone_settings + '.Z', 'Free', False)
            else:
                rig_controller.set_pin_default_value(limit_bone_settings + '.Z', 'Free', False)
            # rig_controller.set_pin_default_value(limit_bone_settings + '.X', 'Limited', False)
            # rig_controller.set_pin_default_value(limit_bone_settings + '.Y', 'Limited', False)
            # rig_controller.set_pin_default_value(limit_bone_settings + '.Z', 'Limited', False)

            # It feels like Min\Max angel aren't the actual extents, but the amount of rotation allowed.  So they shoudl be 0 to abs(min, max)
            # I think there's a bug if a min rotation is more negative than max is positive, the negative gets clamped to the relative positive.
            rig_controller.set_pin_default_value(limit_bone_settings + '.MinX', str(min(bone_limit_x_max, bone_limit_x_min)), False)
            rig_controller.set_pin_default_value(limit_bone_settings + '.MaxX', str(max(bone_limit_x_max, abs(bone_limit_x_min))), False)
            rig_controller.set_pin_default_value(limit_bone_settings + '.MinY', str(min(bone_limit_y_max, bone_limit_y_min)), False)
            rig_controller.set_pin_default_value(limit_bone_settings + '.MaxY', str(max(bone_limit_y_max, abs(bone_limit_y_min))), False)
            rig_controller.set_pin_default_value(limit_bone_settings + '.MinZ', str(min(bone_limit_z_max, bone_limit_z_min)), False)
            rig_controller.set_pin_default_value(limit_bone_settings + '.MaxZ', str(max(bone_limit_z_max, abs(bone_limit_z_min))), False)


            # rig_controller.set_pin_default_value(limit_bone_settings + '.MinX', str(min(bone_limit_z_max, bone_limit_z_min)), False)
            # rig_controller.set_pin_default_value(limit_bone_settings + '.MaxX', str(max(bone_limit_z_max, bone_limit_z_min)), False)
            # rig_controller.set_pin_default_value(limit_bone_settings + '.MinY', str(min(bone_limit_x_max, bone_limit_x_min)), False)
            # rig_controller.set_pin_default_value(limit_bone_settings + '.MaxY', str(max(bone_limit_x_max, bone_limit_x_min)), False)
            # rig_controller.set_pin_default_value(limit_bone_settings + '.MinZ', str(min(bone_limit_y_max, bone_limit_y_min)), False)
            # rig_controller.set_pin_default_value(limit_bone_settings + '.MaxZ', str(max(bone_limit_y_max, bone_limit_y_min)), False)

            if bone_limit_name in stiff_limits:
                rig_controller.set_pin_default_value(limit_bone_settings + '.PositionStiffness', '0.800000', False)
                rig_controller.set_pin_default_value(limit_bone_settings + '.RotationStiffness', '0.800000', False)

            # Figure out preferred angles, the primary angle is the one that turns the furthest from base pose
            rig_controller.set_pin_default_value(limit_bone_settings + '.bUsePreferredAngles', 'true', False)

            x_max_rotate = max(abs(bone_limit_x_min), abs(bone_limit_x_max))
            y_max_rotate = max(abs(bone_limit_y_min), abs(bone_limit_y_max))
            z_max_rotate = max(abs(bone_limit_z_min), abs(bone_limit_z_max))
            #print(bone_limit_name, x_max_rotate, y_max_rotate, z_max_rotate)

            if bone_limit_name in suggested_rotation_list:
                rig_controller.set_pin_default_value(limit_bone_settings + '.PreferredAngles.X', "0.0", False)
                rig_controller.set_pin_default_value(limit_bone_settings + '.PreferredAngles.Y', "0.0", False)
                rig_controller.set_pin_default_value(limit_bone_settings + '.PreferredAngles.Z', "0.0", False)
                create_control(bone_limit_name)
                to_euler_name = "Preferred_Angles_To_Euler_" + bone_limit_name
                get_transform_name = "Preferred_Angles_GetRotator_" + bone_limit_name
                rig_controller.add_unit_node_from_struct_path('/Script/ControlRig.RigUnit_GetControlRotator', 'Execute', unreal.Vector2D(-344.784477, 4040.400172), get_transform_name)
                rig_controller.set_pin_default_value(get_transform_name + '.Space', 'GlobalSpace')
                #rig_controller.set_pin_default_value(get_transform_name + '.bInitial', 'False')
                rig_controller.set_pin_default_value(get_transform_name + '.Control', bone_limit_name + '_ctrl', True)
                #rig_controller.set_pin_default_value(get_transform_name + '.Item.Type', 'Control', True)
                #rig_controller.add_unit_node_from_struct_path('/Script/RigVM.RigVMFunction_MathQuaternionToEuler', 'Execute', unreal.Vector2D(9.501237, 4176.400172), to_euler_name)
                #rig_controller.add_link(get_transform_name + '.Transform.Rotation', to_euler_name + '.Value')
                rig_controller.add_link(get_transform_name + '.Rotator.Roll', limit_bone_settings + '.PreferredAngles.X')
                rig_controller.add_link(get_transform_name + '.Rotator.Pitch', limit_bone_settings + '.PreferredAngles.Y')
                rig_controller.add_link(get_transform_name + '.Rotator.Yaw', limit_bone_settings + '.PreferredAngles.Z')
            else:
                limit_divider = 1.0
                if x_max_rotate > y_max_rotate and x_max_rotate > z_max_rotate:
                    if abs(bone_limit_x_min) > abs(bone_limit_x_max):
                        rig_controller.set_pin_default_value(limit_bone_settings + '.PreferredAngles.X', str(bone_limit_x_min / limit_divider), False)
                    else:
                        rig_controller.set_pin_default_value(limit_bone_settings + '.PreferredAngles.X', str(bone_limit_x_max / limit_divider), False)

                if y_max_rotate > x_max_rotate and y_max_rotate > z_max_rotate:
                    if abs(bone_limit_y_min) > abs(bone_limit_y_max):
                        rig_controller.set_pin_default_value(limit_bone_settings + '.PreferredAngles.Y', str(bone_limit_y_min / limit_divider), False)
                    else:
                        rig_controller.set_pin_default_value(limit_bone_settings + '.PreferredAngles.Y', str(bone_limit_y_max / limit_divider), False)
            
                if z_max_rotate > x_max_rotate and z_max_rotate > y_max_rotate:
                    if abs(bone_limit_z_min) > abs(bone_limit_z_max):
                        rig_controller.set_pin_default_value(limit_bone_settings + '.PreferredAngles.Z', str(bone_limit_z_min / limit_divider), False)
                    else:
                        rig_controller.set_pin_default_value(limit_bone_settings + '.PreferredAngles.Z', str(bone_limit_z_max / limit_divider), False)
            # rig_controller.set_pin_default_value('PBIK.BoneSettings.0.Bone', 'pelvis', False)
            # rig_controller.set_pin_default_value('PBIK.BoneSettings.0.RotationStiffness', '0.900000', False)
            # rig_controller.set_pin_default_value('PBIK.BoneSettings.0.PositionStiffness', '0.900000', False)
        

    # Attach the node to execute
    rig_controller.add_link(next_forward_execute, 'PBIK.ExecuteContext')

    unreal.ControlRigBlueprintLibrary.set_preview_mesh(blueprint, skeletal_mesh)

    # Turn on notifications and force a recompile
    blueprint.suspend_notifications(False)
    unreal.ControlRigBlueprintLibrary.recompile_vm(blueprint)
    #rig_controller.add_link('RigUnit_BeginExecution.ExecuteContext', 'PBIK.ExecuteContext')