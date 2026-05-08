import socket
from urllib.parse import urlparse, urlunparse


def resolve_address(url: str) -> str:
    # Parse the URL into its components.
    parsed = urlparse(url)
    hostname = parsed.hostname
    if hostname is None:
        # If we can't extract a hostname, return the original URL.
        return url

    # Try to resolve the hostname to an IP address.
    try:
        # This uses the OS resolver (which may include mDNS support on some platforms)
        ip_address = socket.gethostbyname(hostname)
    except socket.gaierror:
        # If resolution fails, fallback to the original hostname.
        ip_address = hostname

    # Rebuild the network location (netloc) including any credentials or port.
    new_netloc = ""
    if parsed.username:
        new_netloc += parsed.username
        if parsed.password:
            new_netloc += f":{parsed.password}"
        new_netloc += "@"
    new_netloc += ip_address
    if parsed.port:
        new_netloc += f":{parsed.port}"

    # Reconstruct the URL with the new netloc.
    new_url = urlunparse((parsed.scheme, new_netloc, parsed.path, parsed.params, parsed.query, parsed.fragment))
    return new_url
