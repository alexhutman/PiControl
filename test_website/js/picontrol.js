getSocket = (promptText) => {
    let hostname = prompt(promptText)
    return new WebSocket(hostname)
}

let socket
let firstAttempt = true
while (!socket) {
  let promptText = firstAttempt ? "Enter hostname:" : "Invalid hostname. Try again:"
  if (firstAttempt) {
    firstAttempt = false
  }

  window.setTimeout(() => {console.log("yo")}, 3000)
  try {
    socket = getSocket("Invalid hostname. Try again:")
  }
  catch {
  }
}

let textbox = document.getElementById('picontrol-input')

textbox.addEventListener('keyup', e => {
  console.log(e)
  textbox.value = ""
})
