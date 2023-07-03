# Model Format

I just use some custom JSON format, here's an example

```JSON
{
  "model_name": "box",
  "mesh": "box.obj",
  "albedo": [0.1, 0.2, 0.5],
  "albedo_texture": null,
  "metallic": null,
  "metallic_texture": "box_metallic.png",
  "roughness": 0.2,
  "roughness_texture": null,
  "ao": 0.3,
  "ao_texture": null,
  "normal_map_texture": "box_normal.png"
}
```

The textures are assumed to live relative to the 'textures' folder, models in the 'models' folder