#version 330 core
struct Material { vec3 ambient; vec3 diffuse; vec3 specular; float shininess; };
struct Light    { vec3 position; vec3 ambient; vec3 diffuse; vec3 specular; };

in vec3 FragPos;
in vec3 Normal;
out vec4 FragColor;

uniform Material material;
uniform Light    light;
uniform vec3     viewPos;

void main(){
    vec3 N = normalize(Normal);
    vec3 L = normalize(light.position - FragPos);
    vec3 V = normalize(viewPos - FragPos);

    float diff = max(dot(N,L), 0.0);
    vec3 R = reflect(-L, N);
    float spec = pow(max(dot(V,R), 0.0), material.shininess);

    vec3 ambient  = light.ambient  * material.ambient;
    vec3 diffuse  = light.diffuse  * (diff * material.diffuse);
    vec3 specular = light.specular * (spec * material.specular);

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}
