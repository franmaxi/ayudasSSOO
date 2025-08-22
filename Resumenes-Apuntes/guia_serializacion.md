# Guía de Serialización - Sistemas Operativos UTN

## Introducción

La serialización de paquetes es necesaria porque:

- El emisor del paquete no conoce cuánto espacio ocupará la información a enviar
- El receptor tampoco sabe cuánto espacio debe reservar para recibir el paquete

### Header
Al serializar un paquete, el primer elemento se denomina **header**. Este elemento indica:
- Qué tipo de información estamos enviando
- Qué tipo de datos va a recibir el servidor

Al recibir un paquete, sabemos que comenzará con el **header**, permitiéndonos reservar memoria apropiadamente.

### Proceso de Desempaquetado
Al recibir un paquete, se desempaqueta **desde el último elemento hacia el primero**.

## Estructuras Dinámicas

Cuando el `struct` que queremos enviar contiene punteros, debemos agregar un entero con la longitud del puntero para cada elemento dinámico.

```c
#include <commons/string.h>

typedef struct {
    char* username;
    uint32_t username_length;
    char* message;
    uint32_t message_length;
} t_package;

t_package package_create(char *username, char *message) {
    t_package package;
    package.username = string_duplicate(username);
    package.username_length = string_length(username) + 1; // +1 para el '\0'
    package.message = string_duplicate(message);
    package.message_length = string_length(message) + 1; // +1 para el '\0'
    return package;
}
```

**Explicación**: En este ejemplo, cada string dinámico (`username` y `message`) tiene su correspondiente campo de longitud (`username_length` y `message_length`). Esto permite al receptor saber exactamente cuántos bytes debe leer para cada campo dinámico.

## Ejemplo de Serialización

### Estructura de Ejemplo
```c
typedef struct {
    uint32_t dni;
    uint8_t edad;
    uint32_t pasaporte;
    uint32_t nombre_length;
    char* nombre;
} t_persona;
```

### Buffer de Serialización
```c
typedef struct {
    uint32_t size;      // Tamaño del payload
    uint32_t offset;    // Desplazamiento dentro del payload
    void* stream;       // Payload
} t_buffer;
```

**Explicación del Buffer**:

- **`size`**: Indica el tamaño total en bytes del payload (los datos serializados)
- **`offset`**: Es un "puntero" que indica la posición actual donde estamos escribiendo o leyendo dentro del stream. Comienza en 0 y se incrementa conforme agregamos datos
- **`stream`**: Es el espacio de memoria donde se almacenan todos los datos serializados de forma contigua

**¿Cómo funciona?**
1. Calculamos cuánto espacio necesitamos para todos los datos (size)
2. Reservamos esa memoria (stream)
3. Usamos offset para ir "navegando" por el stream mientras copiamos cada campo
4. Cada vez que copiamos un dato, incrementamos offset por el tamaño copiado

Es como tener una caja (stream) de tamaño conocido (size) y un marcador (offset) que nos dice dónde estamos poniendo el próximo elemento.

### Creación y Llenado del Buffer
```c
t_buffer* buffer = malloc(sizeof(t_buffer));
buffer->size = sizeof(uint32_t) * 3    // DNI, Pasaporte y longitud del nombre
             + sizeof(uint8_t)          // Edad
             + persona.nombre_length;   // La longitud del string nombre.
                                       // Le habíamos sumado 1 para enviar tambien el caracter centinela '\0'.
                                       // Esto se podría obviar, pero entonces deberíamos agregar el centinela en el receptor.

buffer->offset = 0;
buffer->stream = malloc(buffer->size);

memcpy(buffer->stream + buffer->offset, &persona.dni, sizeof(uint32_t));
buffer->offset += sizeof(uint32_t);

memcpy(buffer->stream + buffer->offset, &persona.edad, sizeof(uint8_t));
buffer->offset += sizeof(uint8_t);

memcpy(buffer->stream + buffer->offset, &persona.pasaporte, sizeof(uint32_t));
buffer->offset += sizeof(uint32_t);

// Para el nombre primero mandamos el tamaño y luego el texto en sí:
memcpy(buffer->stream + buffer->offset, &persona.nombre_length, sizeof(uint32_t));
buffer->offset += sizeof(uint32_t);

memcpy(buffer->stream + buffer->offset, persona.nombre, persona.nombre_length);
// No tiene sentido seguir calculando el desplazamiento, ya ocupamos el buffer completo

// Si usamos memoria dinámica para el nombre, y no la precisamos más, ya podemos liberarla:
free(persona.nombre);
```

**Explicación del Llenado del Buffer**:
1. **Cálculo del tamaño**: Sumamos el tamaño de todos los campos fijos (DNI, edad, pasaporte, longitud del nombre) más el tamaño variable del nombre
2. **Inicialización**: Creamos el buffer, establecemos offset en 0 y reservamos memoria para el stream
3. **Serialización secuencial**: Copiamos cada campo al stream usando `memcpy()` y actualizamos el offset
4. **Campos dinámicos**: Para el nombre, primero copiamos su longitud y después el contenido
5. **Liberación temprana**: Podemos liberar la memoria del nombre original ya que está copiado en el buffer

**Nota importante**: El carácter centinela '\0' se puede incluir o no. Si no se incluye, debe agregarse manualmente en el receptor.

### Estructura del Paquete
```c
typedef struct {
    uint8_t codigo_operacion;
    t_buffer* buffer;
} t_paquete;
```

### Envío del Paquete (Serialización)
```c
t_paquete* paquete = malloc(sizeof(t_paquete));

paquete->codigo_operacion = PERSONA; // Podemos usar una constante por operación
paquete->buffer = buffer; // Nuestro buffer de antes.

// Armamos el stream a enviar
void* a_enviar = malloc(buffer->size + sizeof(uint8_t) + sizeof(uint32_t));
int offset = 0;

memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(uint8_t));

offset += sizeof(uint8_t);
memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(uint32_t));
offset += sizeof(uint32_t);
memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

// Por último enviamos
send(unSocket, a_enviar, buffer->size + sizeof(uint8_t) + sizeof(uint32_t), 0);

// No nos olvidamos de liberar la memoria que ya no usaremos
free(a_enviar);
free(paquete->buffer->stream);
free(paquete->buffer);
free(paquete);
```

**Explicación del Envío**:
1. Creamos el paquete con el código de operación y el buffer
2. Reservamos memoria para el stream final que incluye: código_operación + size + payload
3. Copiamos secuencialmente: primero el código, después el tamaño, finalmente los datos
4. Enviamos todo de una vez usando `send()`
5. Liberamos toda la memoria utilizada

### Recepción del Paquete (Deserialización)
```c
t_paquete* paquete = malloc(sizeof(t_paquete));
paquete->buffer = malloc(sizeof(t_buffer));

// Primero recibimos el codigo de operacion
recv(unSocket, &(paquete->codigo_operacion), sizeof(uint8_t), 0);

// Después ya podemos recibir el buffer. Primero su tamaño seguido del contenido
recv(unSocket, &(paquete->buffer->size), sizeof(uint32_t), 0);
paquete->buffer->stream = malloc(paquete->buffer->size);
recv(unSocket, paquete->buffer->stream, paquete->buffer->size, 0);

// Ahora en función del código recibido procedemos a deserializar el resto
switch(paquete->codigo_operacion) {
    case PERSONA:
        t_persona* persona = persona_serializar(paquete->buffer);
        ...
        // Hacemos lo que necesitemos con esta info
        // Y eventualmente liberamos memoria
        free(persona);
        ...
        break;
    ... // Evaluamos los demás casos según corresponda
}

// Liberamos memoria
free(paquete->buffer->stream);
free(paquete->buffer);
free(paquete);
```

**Explicación de la Recepción**:
1. Recibimos primero el código de operación (sabemos que siempre es 1 byte)
2. Después recibimos el tamaño del payload (siempre 4 bytes para uint32_t)
3. Con el tamaño conocido, reservamos memoria y recibimos el payload completo
4. Usando un switch, deserializamos según el tipo de operación
5. Liberamos toda la memoria utilizada

### Función de Deserialización
```c
t_persona* persona_serializar(t_buffer* buffer) {
    t_persona* persona = malloc(sizeof(t_persona));

    void* stream = buffer->stream;
    // Deserializamos los campos que tenemos en el buffer
    memcpy(&(persona->dni), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);
    memcpy(&(persona->edad), stream, sizeof(uint8_t));
    stream += sizeof(uint8_t);
    memcpy(&(persona->pasaporte), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);

    // Por último, para obtener el nombre, primero recibimos el tamaño y luego el texto en sí:
    memcpy(&(persona->nombre_length), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);
    persona->nombre = malloc(persona->nombre_length);
    memcpy(persona->nombre, stream, persona->nombre_length);

    return persona;
}
```

**Explicación de la Deserialización**:
1. Usamos un puntero `stream` que se va moviendo por el buffer
2. Copiamos cada campo en orden: dni, edad, pasaporte
3. Para el nombre (campo dinámico): primero leemos su longitud, después reservamos memoria y copiamos el contenido
4. El puntero `stream` se incrementa con el tamaño de cada campo copiado
5. Retornamos la estructura completamente reconstruida

## Código en C - Manejo de Memoria

### Reservar Memoria
```c
void* malloc(size_t size);
```

### Calcular Memoria a Reservar
```c
sizeof(Estructura/TipoAReservar)
```

### Liberar Memoria
```c
void free(void* ptr);
```

**Explicación**: 
- `malloc()` reserva un bloque de memoria del tamaño especificado
- `sizeof()` calcula automáticamente el tamaño en bytes de una estructura o tipo de dato
- `free()` libera la memoria previamente reservada con `malloc()`

**Importante**: Al liberar memoria de un struct que contiene punteros internos, debemos liberar **desde lo particular a lo general**:
1. Primero liberar los elementos internos del struct (los punteros)
2. Después liberar el struct completo

### Función para Copiar Memoria
```c
void memcpy(void* dest, const void* src, size_t n);
```

**Explicación**: Copia `n` bytes desde la ubicación de memoria `src` hacia `dest`. Es fundamental para serializar datos en el buffer.