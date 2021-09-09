
getSocket = (promptText) => {
    let hostname = prompt(promptText)
    return new WebSocket(hostname)
}

let socket
firstAttempt = true
while (!socket) {
  promptText = firstAttempt ? "Enter hostname:" : "Invalid hostname. Try again:"
  if (firstAttempt) {
    firstAttempt = false
  }

  try {
    socket = getSocket("Invalid hostname. Try again:")
  }
  catch {
  }
}

let textbox = document.getElementById('picontrol-input')

textbox.addEventListener('keyup', e => {
  console.log(e)
  textbox.value = textbox.value.slice(0, -1)
})