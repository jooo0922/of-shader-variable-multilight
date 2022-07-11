#version 410

// c++ 미리 계산된 후 받아온 조명연산에 필요한 유니폼 변수들
uniform vec3 lightPos; // 포인트라이트 위치
uniform vec3 lightCol; // 조명 색상
uniform float lightRadius; // 포인트라이트 조명의 반경(최대범위)
uniform vec3 cameraPos; // 각 프래그먼트 -> 카메라 방향의 벡터 (이하 '뷰 벡터' 또는 '카메라 벡터') 계산에 필요한 카메라 월드공간 좌표
uniform vec3 ambientCol; // 앰비언트 라이트(환경광 또는 글로벌 조명(전역 조명))의 색상 

// 물 셰이더를 만들기 위해 필요한 노말맵을 전달받는 유니폰 변수들
uniform sampler2D normTex;
uniform sampler2D normTex2; // 이 예제에서는 샘플링할 uv좌표를 달리 계산해서 서로 다른 노멀(벡터)를 구하므로, 두 번째 노멀맵이 따로 필요하진 않음.

uniform samplerCube envMap; // 환경광 반사를 물 셰이더에 적용하기 위해 필요한 큐브맵(환경맵)을 전달받는 유니폼 변수

in vec3 fragNrm; // 버텍스 셰이더에서 받아온 물 표면 모델의 (월드공간) 노멀벡터가 보간되어 들어온 값
in vec3 fragWorldPos; // 버텍스 셰이더에서 받아온 물 표면 모델의 월드공간 위치 좌표가 보간되어 들어온 값

// 각 버텍스마다 서로 다르게 계산된 uv좌표가 보간되어 들어온 값 (얘내들로 노말맵을 샘플링하므로, 서로 다른 탄젠트 공간 노멀벡터를 샘플링해올 것임)
in vec2 fragUV;
in vec2 fragUV2;

in mat3 TBN; // 노말맵 텍스쳐에서 샘플링한 탄젠트 공간의 노말벡터를 월드공간으로 변환하기 위해, 버텍스 셰이더에서 각 버텍스마다 계산한 뒤, 보간되어 들어온 TBN 행렬

out vec4 outCol; // 최종 출력할 색상을 계산하여 다음 파이프라인으로 넘겨줄 변수

// 디렉셔널 라이트, 포인트 라이트, 스포트 라이트 등 어떤 라이트 유형이라도 가져다 쓸 수 있도록 블린-퐁 라이트의 각 요소들을 계산하는 별도 함수를 추출함 -> 일종의 리팩토링
// 디퓨즈 라이팅 계산 (노멀벡터와 조명벡터를 내적)
float diffuse(vec3 lightDir, vec3 normal) {
  float diffAmt = max(0.0, dot(normal, lightDir)); // 정규화된 노멀벡터와 조명벡터의 내적값을 구한 뒤, max() 함수로 음수인 내적값 제거.
  return diffAmt;
}

// Blinn-Phong 공식에서의 스펙큘러 라이팅 계산
float specular(vec3 lightDir, vec3 viewDir, vec3 normal, float shininess) {
  vec3 halfVec = normalize(viewDir + lightDir); // 뷰 벡터와 조명벡터 사이의 하프벡터를 구함
  float specAmt = max(0.0, dot(halfVec, normal)); // 하프벡터와 노멀벡터의 내적값을 구한 뒤, max() 함수로 음수값 제거
  return pow(specAmt, shininess); // 물 재질은 거울처럼 반사가 아주 세므로, 스펙큘러 계산 시 광택지수를 512 처럼 높게 잡아줘야 함.
}

void main(){
  // 아래의 normal, normal2 는 동일한 텍스쳐를 사용하지만,
  // 샘플링하는 uv좌표가 다르므로, 결과적으로 서로 다른 (탄젠트 공간의)노말벡터를 리턴받음.
  vec3 normal = texture(normTex, fragUV).rgb;
  normal = (normal * 2.0 - 1.0); // 노말맵에서 샘플링한 텍셀값 범위 0 ~ 1 을 탄젠트 공간의 노말벡터가 속한 좌표계 범위인 -1 ~ 1 로 맵핑함.

  vec3 normal2 = texture(normTex, fragUV2).rgb;
  normal2 = (normal2 * 2.0 - 1.0); // 노말맵에서 샘플링한 텍셀값 범위 0 ~ 1 을 탄젠트 공간의 노말벡터가 속한 좌표계 범위인 -1 ~ 1 로 맵핑함.

  normal = normalize(TBN * (normal + normal2)); // 두 탄젠트 공간의 노멀벡터를 더해서 그 사이의 하프벡터를 구하고, 그걸 TBN 행렬과 곱해 월드공간의 노말벡터로 변환함.
  // 여기까지 해야 노말맵에서 샘플링해온 노말벡터는 조명계산에 써먹을 수 있는 상태가 되었고, 이후의 계산은 원래 하던 blinn-phong 계산과 동일하게 수행하면 됨.

  vec3 viewDir = normalize(cameraPos - fragWorldPos); // 카메라의 월드공간 좌표 - 각 프래그먼트 월드공간 좌표를 빼서 각 프래그먼트 -> 카메라 방향의 벡터인 뷰 벡터 계산

  vec3 envSample = texture(envMap, reflect(-viewDir, normal)).xyz; // 큐브맵 텍스쳐로부터 방향벡터를 사용해 샘플링한 텍셀값

  vec3 finalColor = vec3(0.0, 0.0, 0.0); // 최종 포인트라이트값은 여기에다 계산 할거임.

  // 포인트라이트 계산
  // 포인트 라이트 방향벡터 및 감쇄값 계산
  vec3 toLight = lightPos - fragWorldPos; // 포인트라이트 월드공간 위치 ~ 각 프래그먼트 월드공간 위치까지의 벡터 계산
  vec3 lightDir = normalize(toLight); // 위에서 구한 각 프래그먼트에 도달하는 포인트라이트 방향벡터의 길이를 1로 맞춰서 방향벡터 구함.
  float distToLight = length(toLight); // 정규화되지 않은 각 프래그먼트에 도달하는 포인트라이트 벡터의 길이값, 즉, 각 프래그먼트에서 조명까지의 거리값을 구해놓음.
  float falloff = 1.0 - (distToLight / lightRadius); // 각 프래그먼트에서 조명까지의 거리값을 포인트라이트 조명의 반경(최대범위)로 나눈 뒤, 1에서 빼줌으로써 감쇄값을 계산함.
  // 왜냐하면, 프래그먼트가 포인트라이트에 가까울수록 (distToLight / lightRadius) 는 0에 가깝겠지만, specAmt, diffAmt 에 곱해주는 감쇄값인 falloff 는 가까울수록 1에 가까워야 할테니
  // 1에서 빼줘서 값을 뒤집어준 것임!

  // 디렉셔널 라이트와 다르게 디퓨즈 라이팅값과 스펙큘러 라이팅값 계산 이후 위에서 구한 '감쇄값' 을 추가로 곱해줘야 함!
  float diffAmt = diffuse(lightDir, normal) * falloff; // 별도로 추출한 함수로부터 디퓨즈 라이팅 값 리턴받음.
  float specAmt = specular(lightDir, viewDir, normal, 512.0) * falloff; // 별도로 추출한 함수로부터 스펙큘러 라이팅 값 리턴받음.

  vec3 specCol = lightCol * specAmt; // 최종 스펙큘러 라이트 색상값을 계산함.
  vec3 diffCol = envSample * lightCol * diffAmt; // 큐브맵 텍스쳐에서 샘플링한 색상을 곱해 최종 디퓨즈 라이트 색상값을 계산함.
  // 물 색상은 주변 환경맵의 색상을 비춘 색상으로 나오므로 shiledMesh 와 달리, 환경맵 색상을 보정없이 그대로 곱해줘도 됨.

  finalColor += diffCol; // 디퓨즈 라이트 색상값을 최종 색상에 더해줌.
  finalColor += specCol; // 스펙큘러 라이트 색상값을 최종 색상에 더해줌.

  outCol = vec4(finalColor + ambientCol, 1.0); // '각 라이트 유형별 라이팅값이 누적계산된 최종색상값 + 앰비언트 라이트 색상값' 을 합쳐서 최종최종 색상값 결정
}

/*
  환경광 반사 계산에 필요한 
  환경맵 샘플링 시 카메라 벡터와 노말벡터로 구한 반사벡터를 사용하는 이유

  원래 reflect() 함수는 조명의 반사벡터를 구하기 위해 첫 번째 인자로 
  조명벡터를 넣어주지만, 여기서는 카메라 벡터를 넣어주고 있지?

  그 이유를 추론컨데,
  환경맵 전체에서 들어오는 빛을 계산하기 어려우니,
  물 셰이더의 각각의 픽셀에서 반사되어 카메라로 들어오는
  광선들을 '역추적'해서 큐브맵을 샘플링할 방향벡터를 구한다고 생각하면 됨.

  '레이 트레이싱'과 유사한 원리를 사용하고 있으며,
  실제로 환경맵을 '레이트레이싱의 결과물을 흉내내는(simulate)'
  것이라고 설명하기도 함. 
*/