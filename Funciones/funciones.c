typedef struct {
    uint32_t size; // Tamaño del payload
    uint32_t offset; // Desplazamiento dentro del payload
    void* stream; // Payload
} t_buffer;
// Crea un buffer vacío de tamaño size y offset 0
t_buffer *buffer_create(uint32_t size){
    t_buffer* buffer= malloc(sizeof(t_buffer));
    if(buffer == NULL){
        log_error(nombreLog,"Error al reservar memoria para la creacion del buffer");
        return NULL;
    }
    buffer->size = size;
    buffer->offset = 0;
    buffer->stream= malloc(size);
    if(stream == NULL){
        log_error(nombreLog,"Error al reservar memoria para el stream del buffer");
        free(buffer);
        return NULL;
    }
    return buffer;
}

// Agrega un uint32_t al buffer
void buffer_add_uint32(t_buffer *buffer, uint32_t data){
    if(buffer->offset+sizeof(uint32_t) <= buffer->size){
        memcpy(buffer->stream + buffer->offset, &data, sizeof(uint32_t));
        buffer->offset += sizeof(uint32_t);
    }
    else{
        log_error(nombreLog,"Error al agregar un int32 al buffer");
    }
}

void buffer_add_uint8(t_buffer *buffer, uint8_t data){
    if(buffer->offset + sizeof(uint8_t) <= buffer->size){
        memcpy(buffer->stream + buffer->offset, &data, sizeof(uint8_t));
        buffer->offset += sizeof(uint8_t);
 
    }
    else{
        log_error(nombreLog,"Error al agregar un int8 al buffer");
    }
}
void buffer_add_string(t_buffer *buffer, uint32_t length, char *string){
    if(buffer->offset + sizeof(uint32_t) + length <= buffer->size){
        
        memcpy(buffer->stream + buffer->offset, &length, sizeof(uint32_t));
        buffer->offset += sizeof(uint32_t);
        
        memcpy(buffer->stream + buffer->offset, string, length);
        buffer->offset += length;

    }
    else{
        log_error(nombreLog,"Error al agregar un string al buffer");
    }
}

uint32_t buffer_read_uint32(t_buffer *buffer){
    if(buffer->offset + sizeof(uint32_t) <= buffer->size) {
        uint32_t data;
        memcpy(&data, buffer->stream + buffer->offset, sizeof(uint32_t));
        buffer->offset += sizeof(uint32_t);
        return data;
    }
    else {
        log_error(nombreLog, "Intento de lectura fuera del buffer");
        return -1;
    }
}

uint8_t buffer_read_uint8(t_buffer *buffer){
   if(buffer->offset + sizeof(uint8_t) <= buffer->size) {
        uint8_t data;
        memcpy(&data, buffer->stream + buffer->offset, sizeof(uint8_t));
        buffer->offset += sizeof(uint8_t);
        return data;
    }
    else {
        log_error(nombreLog, "Intento de lectura fuera del buffer");
        return -1;
    }
}
char *buffer_read_string(t_buffer *buffer, uint32_t *length){
    if(buffer->offset + sizeof(uint32_t) > buffer->size) {
        log_error(nombreLog, "Intento de lectura fuera del buffer");
        return NULL;
    }
    memcpy(length, buffer->stream + buffer->offset, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    //verificacion de que podemos leer el string
    if(buffer->offset + *length > buffer->size) {
        log_error(nombreLog, "String excede la longitud del buffer");
        return NULL;
    }
    char* data = malloc(*length);
    if(data == NULL){
        log_error(nombreLog,"Error al pedir memoria para desempaquetar un string");
        return NULL;
    }
    memcpy(data, buffer->stream + buffer->offset, *length);
    buffer->offset += *length;
    return data;
    /*  Ojo! que estamos haciendo un malloc, despues de usar esto tenemos que 
        liberar la memoria...
    */
}

void buffer_destroy(t_buffer *buffer){
    if(buffer == NULL){
        log_warning(nombreLog,"El buffer ya fue liberado");
    }
    free(buffer->stream);
    free(buffer);
}