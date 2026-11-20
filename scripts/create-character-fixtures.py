"""Run with Blender --background --factory-startup --python <this file>."""

from pathlib import Path
import bpy


DESTINATION = Path(__file__).resolve().parents[1] / "assets" / "characters"


def cube(name, center, size, color):
    bpy.ops.mesh.primitive_cube_add(size=1, location=center)
    part = bpy.context.object
    part.name = name
    part.scale = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    attribute = part.data.color_attributes.new(name="Color", type="FLOAT_COLOR", domain="CORNER")
    for value in attribute.data:
        value.color = (*color, 1)
    part.data.color_attributes.active_color = attribute
    for polygon in part.data.polygons:
        polygon.use_smooth = False
    return part


def create_model(name, zombie):
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    skin = (0.22, 0.48, 0.12) if zombie else (0.70, 0.39, 0.20)
    shirt = (0.25, 0.15, 0.38) if zombie else (0.04, 0.24, 0.65)
    trousers = (0.09, 0.12, 0.10) if zombie else (0.10, 0.14, 0.25)
    parts = {
        "left_leg": cube("left_leg", (-0.14, 0, 0.37), (0.24, 0.28, 0.74), trousers),
        "right_leg": cube("right_leg", (0.14, 0, 0.37), (0.24, 0.28, 0.74), trousers),
        "body": cube("body", (0, 0, 1.03), (0.52, 0.30, 0.58), shirt),
        "head": cube("head", (0, 0, 1.56), (0.45, 0.40, 0.48), skin),
        "left_arm": cube("left_arm", (-0.35, 0, 1.02), (0.18, 0.24, 0.60), skin),
        "right_arm": cube("right_arm", (0.35, 0, 1.02), (0.18, 0.24, 0.60), skin),
    }
    # Blender -Y maps to GLB +Z, the model's front. Face detail belongs to head.
    details = [
        cube("left_eye", (-0.105, -0.207, 1.61), (0.07, 0.025, 0.07), (0.015, 0.015, 0.015)),
        cube("right_eye", (0.105, -0.207, 1.61), (0.07, 0.025, 0.07), (0.015, 0.015, 0.015)),
    ]
    if zombie:
        details.append(cube("mouth", (0, -0.207, 1.43), (0.20, 0.025, 0.06), (0.03, 0.08, 0.02)))
    else:
        details.append(cube("hair", (0, 0.02, 1.79), (0.46, 0.40, 0.10), (0.08, 0.035, 0.015)))
    bpy.ops.object.select_all(action="DESELECT")
    for detail in [parts["head"], *details]:
        detail.select_set(True)
    bpy.context.view_layer.objects.active = parts["head"]
    bpy.ops.object.join()
    pivots = {
        "body": (0, 0, 0.74), "head": (0, 0, 1.32),
        "left_arm": (-0.35, 0, 1.32), "right_arm": (0.35, 0, 1.32),
        "left_leg": (-0.14, 0, 0.74), "right_leg": (0.14, 0, 0.74),
    }
    root = bpy.data.objects.new("symocraft_rigid_v1", None)
    bpy.context.collection.objects.link(root)
    for part_name, part in parts.items():
        bpy.ops.object.select_all(action="DESELECT")
        part.select_set(True)
        bpy.context.view_layer.objects.active = part
        bpy.context.scene.cursor.location = pivots[part_name]
        bpy.ops.object.origin_set(type="ORIGIN_CURSOR")
        world = part.matrix_world.copy()
        part.parent = root if part_name == "body" else parts["body"]
        part.matrix_world = world
        # Explicit local origin is the pivot; this child marker defines its axis.
        axis = bpy.data.objects.new(part_name + "_axis", None)
        bpy.context.collection.objects.link(axis)
        axis.parent = part
        axis.location = (1, 0, 0)
    bpy.ops.object.select_all(action="SELECT")
    bpy.context.preferences.filepaths.save_version = 0
    bpy.ops.wm.save_as_mainfile(filepath=str(DESTINATION / f"{name}.blend"))
    bpy.ops.export_scene.gltf(
        filepath=str(DESTINATION / f"{name}.glb"), export_format="GLB",
        use_selection=True, export_yup=True, export_normals=True,
        export_materials="NONE", export_vertex_color="ACTIVE",
        export_all_vertex_colors=False, export_active_vertex_color_when_no_material=True,
        export_animations=False, export_skins=False, export_morph=False,
    )


DESTINATION.mkdir(parents=True, exist_ok=True)
create_model("human", False)
create_model("zombie", True)
