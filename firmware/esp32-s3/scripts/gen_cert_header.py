"""
gen_cert_header.py — PlatformIO pre-build script
Convierte firmware/esp32-s3/certs/ca.crt → include/ca_cert.h
Se ejecuta automáticamente antes de compilar.
"""
import os
Import("env")  # noqa: F821  (PlatformIO injects this)

CERT_SRC  = os.path.join(env.subst("$PROJECT_DIR"), "certs", "ca.crt")
CERT_DEST = os.path.join(env.subst("$PROJECT_DIR"), "include", "ca_cert.h")


def generate_cert_header(source, target, env):
    if not os.path.isfile(CERT_SRC):
        print(f"\n[gen_cert_header] ADVERTENCIA: {CERT_SRC} no encontrado.")
        print("[gen_cert_header] Copiar el certificado CA del broker MQTT a ese path.\n")
        # Genera un header de placeholder para que compile
        with open(CERT_DEST, "w") as f:
            f.write(
                "// PLACEHOLDER — reemplazar con el contenido real de ca.crt\n"
                "#pragma once\n"
                "static const char CA_CERT_PEM[] =\n"
                "    \"-----BEGIN CERTIFICATE-----\\n\"\n"
                "    \"REEMPLAZAR_CON_CERT_REAL\\n\"\n"
                "    \"-----END CERTIFICATE-----\\n\";\n"
            )
        return

    with open(CERT_SRC, "r") as f:
        cert_content = f.read().strip()

    # Convierte cada línea en una cadena C entre comillas
    lines = cert_content.splitlines()
    c_lines = ['    "' + line + '\\n"' for line in lines]
    cert_c_str = "\n".join(c_lines) + ";\n"

    header = (
        "// AUTO-GENERADO por scripts/gen_cert_header.py — NO EDITAR\n"
        "// Fuente: certs/ca.crt\n"
        "#pragma once\n"
        "\n"
        "static const char CA_CERT_PEM[] =\n"
        + cert_c_str
    )

    with open(CERT_DEST, "w") as f:
        f.write(header)

    print(f"[gen_cert_header] ca_cert.h generado ({len(lines)} líneas)")


env.AddPreAction("buildprog", generate_cert_header)
generate_cert_header(None, None, env)
